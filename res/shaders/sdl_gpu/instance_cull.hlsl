// Per-instance frustum + occlusion culling: one thread per instance.
// Transforms the submesh's local-space AABB by that instance's model matrix,
// tests it against the camera frustum (Gribb/Hartmann planes, see
// Frustum.cpp), then against a Hi-Z depth pyramid built from LAST FRAME's
// depth buffer, and for survivors atomically appends the instance's
// original index into visible_instance_indices and increments the
// corresponding IndirectDrawCommand's num_instances. This replaces an
// O(instance count) CPU loop (SDL_GPUCullingUtils::compute_batch_mesh_world_bounds)
// with one GPU dispatch per BATCH (covering every submesh in that batch), so
// cost no longer scales with instance count on the CPU, and dispatch count
// no longer scales with submesh count either (see SDL_GPUInstanceCullPass).
//
// One dispatch covers many submeshes: each thread group looks itself up in
// group_to_submesh (indexed by group_to_submesh_base + SV_GroupID.x) to find
// its submesh's metadata, then recovers its LOCAL instance index within that
// submesh via (global_group_index - metadata.first_group) * 64 +
// SV_GroupThreadID.x - group counts vary per submesh, so this is not simply
// SV_DispatchThreadID.x.
//
// The staleness lives entirely in the Hi-Z pyramid's depth values (built
// from last frame's render), NOT in where on screen we look: objects are
// projected with THIS frame's view-projection (current_view_projection), so
// we always sample last frame's depth at the object's actual current screen
// position. Projecting with a stale view-projection would additionally get
// the screen location wrong on any camera motion, compounding a second error
// on top of the intentional one-frame-stale depth data.
//
// The occlusion test has ~1 frame of latency (an object exposed by a moving
// occluder may still be drawn one frame late) - an accepted tradeoff to
// avoid restructuring the frame into two draw phases.
//
// SDL_GPU compute HLSL register convention: space0 = read-only t/s,
// space1 = read-write u, space2 = uniform b.

// Memory layout must exactly match SDL_GPUIndexedIndirectDrawCommand (see
// SDL_gpu.h) and Luminol::Graphics::SDL_GPU::IndirectDrawCommand
// (SDL_GPUCullingUtils.hpp).
struct IndirectDrawCommand {
    uint num_indices;
    uint num_instances;
    uint first_index;
    int vertex_offset;
    uint first_instance;
};

// hiz_pyramid/hiz_sampler must share the same numeric index (t0/s0) to
// compile as one combined-image-sampler descriptor; instance_models is a
// separate SRV so it takes the next free t-register (t and Texture SRVs
// share the same register file in HLSL, so it can't also be t0).
// submesh_metadata/group_to_submesh follow at t2/t3.
Texture2D hiz_pyramid : register(t0, space0);
SamplerState hiz_sampler : register(s0, space0);
StructuredBuffer<row_major float4x4> instance_models : register(t1, space0);

// One entry per submesh. Mirrors struct SubmeshCullMetadata in
// SDL_GPUInstanceCullPass.cpp exactly.
struct SubmeshCullMetadata {
    float4 local_bounds_min;
    float4 local_bounds_max;
    uint command_index;
    uint instance_base_offset;
    uint instance_count;
    uint first_group;
};
StructuredBuffer<SubmeshCullMetadata> submesh_metadata : register(t2, space0);
// One entry per thread group in this dispatch, indexed by
// group_to_submesh_base + SV_GroupID.x: which submesh (index into
// submesh_metadata) that group belongs to.
StructuredBuffer<uint> group_to_submesh : register(t3, space0);

RWStructuredBuffer<IndirectDrawCommand> indirect_commands : register(u0, space1);
RWStructuredBuffer<uint> visible_instance_indices : register(u1, space1);

// frustum_planes: left, right, bottom, top, near, far. current_view_projection:
// THIS frame's view * projection - same matrix used to build
// camera_frustum_planes - for projecting this frame's world AABB to its
// actual current screen position (only the sampled Hi-Z depth data is
// last-frame-stale, not the screen location). hiz_mip_levels: 0 disables the
// occlusion test (first frame / just resized, no valid previous depth).
// group_to_submesh_base: this dispatch's (i.e. this batch's) slice offset
// into group_to_submesh - every other batch dispatched this cull() call has
// its own base, into the same shared buffer. hiz_pyramid_size: mip-0
// width/height, for screen-rect -> mip selection.
cbuffer InstanceCullParams : register(b0, space2) {
    float4 frustum_planes[6];
    row_major float4x4 current_view_projection;
    uint hiz_mip_levels;
    uint group_to_submesh_base;
    float2 hiz_pyramid_size;
};

bool aabb_in_frustum(float3 box_min, float3 box_max) {
    for (uint i = 0; i < 6; ++i) {
        float4 plane = frustum_planes[i];
        float3 positive_vertex = float3(
            plane.x >= 0.0 ? box_max.x : box_min.x,
            plane.y >= 0.0 ? box_max.y : box_min.y,
            plane.z >= 0.0 ? box_max.z : box_min.z
        );
        float distance = dot(plane.xyz, positive_vertex) + plane.w;
        if (distance < 0.0) {
            return false;
        }
    }
    return true;
}

[numthreads(64, 1, 1)]
void main(uint3 group_id : SV_GroupID, uint3 group_thread_id : SV_GroupThreadID) {
    uint global_group_index = group_to_submesh_base + group_id.x;
    uint submesh_index = group_to_submesh[global_group_index];
    SubmeshCullMetadata metadata = submesh_metadata[submesh_index];

    // Groups aren't 1:1 with submeshes - group counts vary per submesh, so
    // this submesh's own group range starts at metadata.first_group, not at
    // group 0 of the dispatch.
    uint local_group_index = global_group_index - metadata.first_group;
    uint instance_index = (local_group_index * 64) + group_thread_id.x;
    if (instance_index >= metadata.instance_count) {
        return;
    }

    row_major float4x4 model = instance_models[instance_index];

    float3 local_bounds_min = metadata.local_bounds_min.xyz;
    float3 local_bounds_max = metadata.local_bounds_max.xyz;
    float3 mid = (local_bounds_min + local_bounds_max) * 0.5;

    // 8 corners + 6 face centers + 12 edge midpoints, so the occlusion test
    // below has more coverage than just the extremal corners - a cube
    // mostly hidden behind nearer neighbors, with only a thin sliver
    // actually visible through a gap, can have that sliver miss the
    // corners and face centers but still land near an edge midpoint.
    float3 sample_points[26] = {
        float3(local_bounds_min.x, local_bounds_min.y, local_bounds_min.z),
        float3(local_bounds_max.x, local_bounds_min.y, local_bounds_min.z),
        float3(local_bounds_min.x, local_bounds_max.y, local_bounds_min.z),
        float3(local_bounds_max.x, local_bounds_max.y, local_bounds_min.z),
        float3(local_bounds_min.x, local_bounds_min.y, local_bounds_max.z),
        float3(local_bounds_max.x, local_bounds_min.y, local_bounds_max.z),
        float3(local_bounds_min.x, local_bounds_max.y, local_bounds_max.z),
        float3(local_bounds_max.x, local_bounds_max.y, local_bounds_max.z),
        // Face centers.
        float3(local_bounds_min.x, mid.y, mid.z),
        float3(local_bounds_max.x, mid.y, mid.z),
        float3(mid.x, local_bounds_min.y, mid.z),
        float3(mid.x, local_bounds_max.y, mid.z),
        float3(mid.x, mid.y, local_bounds_min.z),
        float3(mid.x, mid.y, local_bounds_max.z),
        // Edge midpoints: 4 on the min.z face, 4 on the max.z face, 4
        // connecting them.
        float3(mid.x, local_bounds_min.y, local_bounds_min.z),
        float3(local_bounds_min.x, mid.y, local_bounds_min.z),
        float3(local_bounds_max.x, mid.y, local_bounds_min.z),
        float3(mid.x, local_bounds_max.y, local_bounds_min.z),
        float3(mid.x, local_bounds_min.y, local_bounds_max.z),
        float3(local_bounds_min.x, mid.y, local_bounds_max.z),
        float3(local_bounds_max.x, mid.y, local_bounds_max.z),
        float3(mid.x, local_bounds_max.y, local_bounds_max.z),
        float3(local_bounds_min.x, local_bounds_min.y, mid.z),
        float3(local_bounds_max.x, local_bounds_min.y, mid.z),
        float3(local_bounds_min.x, local_bounds_max.y, mid.z),
        float3(local_bounds_max.x, local_bounds_max.y, mid.z),
    };

    float3 world_samples[26];
    for (int i = 0; i < 26; ++i) {
        world_samples[i] = mul(float4(sample_points[i], 1.0), model).xyz;
    }

    // Only the 8 true corners (not the face centers) are needed for a
    // correct AABB frustum bound.
    float3 world_min = world_samples[0];
    float3 world_max = world_samples[0];
    for (int i = 1; i < 8; ++i) {
        world_min = min(world_min, world_samples[i]);
        world_max = max(world_max, world_samples[i]);
    }

    if (!aabb_in_frustum(world_min, world_max)) {
        return;
    }

    if (hiz_mip_levels > 0) {
        // Reproject the 8 corners + 6 face centers + 12 edge midpoints with
        // THIS frame's camera. A cube viewed obliquely has a hexagonal
        // silhouette, so a bounding rect's own synthetic corners aren't
        // necessarily real points on the object - sampling there can land
        // on a different, genuinely-nearer neighbor and falsely cull an
        // object that has a clear line of sight everywhere except that one
        // synthetic point. Sampling at the object's real surface points
        // instead guarantees every tested position is legitimately part of
        // this object's own footprint; the extra face/edge points add
        // coverage beyond just the 8 extremal corners, catching thin
        // visible slivers the corners alone would miss.
        float nearest_ndc_z = 1e30;
        float2 sample_uv[26];
        bool behind_camera = false;
        for (int j = 0; j < 26; ++j) {
            float4 clip = mul(float4(world_samples[j], 1.0), current_view_projection);
            if (clip.w <= 0.0) {
                behind_camera = true;
                break;
            }
            float3 ndc = clip.xyz / clip.w;
            nearest_ndc_z = min(nearest_ndc_z, ndc.z);
            // NDC xy in [-1,1] -> UV in [0,1] (uv.y=0 at the top, matching
            // fullscreen_vert.hlsl's own inverse mapping).
            sample_uv[j] = float2(ndc.x * 0.5 + 0.5, 0.5 - ndc.y * 0.5);
        }

        // clip.w <= 0 means a sample point is behind the camera (e.g. an
        // object straddling the near plane) - reprojection is unreliable
        // there, so conservatively treat the instance as visible rather
        // than risk a false occlusion cull.
        if (!behind_camera) {
            // Sample at each of the object's 26 real surface points (not
            // synthetic bounding-rect corners), at mip 0 (the finest
            // level). This is exact point sampling, not area coverage - a
            // coarser mip would average over a wide neighborhood around
            // each point and reintroduce exactly the "pulls in a
            // neighboring object's depth" problem this approach is meant
            // to avoid. Take the max stored depth across all samples:
            // conservative (only culls if EVERYTHING actually stored at
            // this object's own surface points is nearer than its own
            // nearest point), and every sample is a genuine point on the
            // object.
            float stored_depth = -1e30;
            for (int k = 0; k < 26; ++k) {
                stored_depth = max(
                    stored_depth,
                    hiz_pyramid.SampleLevel(hiz_sampler, sample_uv[k], 0.0).r
                );
            }

            // Bias absorbs floating-point noise between the hardware
            // rasterizer's depth write (during the actual render) and this
            // shader's independent software reprojection of the same
            // point - without it, an object sitting exactly on its own
            // previously-written depth is a coin-flip self-comparison and
            // flickers (classic "depth acne", same class of issue shadow
            // mapping guards against with a bias).
            const float hiz_depth_bias = 0.0005;
            if (nearest_ndc_z > stored_depth + hiz_depth_bias) {
                return;  // occluded
            }
        }
    }

    uint dest_slot;
    InterlockedAdd(indirect_commands[metadata.command_index].num_instances, 1, dest_slot);
    visible_instance_indices[metadata.instance_base_offset + dest_slot] = instance_index;
}
