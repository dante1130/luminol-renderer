// Per-instance frustum + occlusion culling: one thread per instance.
// Transforms the submesh's local-space AABB by that instance's model matrix,
// tests it against the camera frustum (Gribb/Hartmann planes, see
// Frustum.cpp), then against a Hi-Z depth pyramid built from LAST FRAME's
// depth buffer, and for survivors atomically appends the instance's
// original index into visible_instance_indices and increments the
// corresponding IndirectDrawCommand's num_instances. This replaces an
// O(instance count) CPU loop (SDL_GPUCullingUtils::compute_batch_mesh_world_bounds)
// with one GPU dispatch per submesh, so cost no longer scales with instance
// count on the CPU (see SDL_GPUInstanceCullPass).
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
Texture2D hiz_pyramid : register(t0, space0);
SamplerState hiz_sampler : register(s0, space0);
StructuredBuffer<row_major float4x4> instance_models : register(t1, space0);

RWStructuredBuffer<IndirectDrawCommand> indirect_commands : register(u0, space1);
RWStructuredBuffer<uint> visible_instance_indices : register(u1, space1);

// frustum_planes: left, right, bottom, top, near, far. local_bounds_min/max:
// this submesh's local-space AABB. current_view_projection: THIS frame's
// view * projection - same matrix used to build camera_frustum_planes -
// for projecting this frame's world AABB to its actual current screen
// position (only the sampled Hi-Z depth data is last-frame-stale, not the
// screen location). command_index: this submesh's slot in indirect_commands.
// instance_base_offset: this submesh's slice base in
// visible_instance_indices. instance_count: this batch's instance count
// (thread count ceiling). hiz_mip_levels: 0 disables the occlusion test
// (first frame / just resized, no valid previous depth).
// hiz_pyramid_size: mip-0 width/height, for screen-rect -> mip selection.
cbuffer InstanceCullParams : register(b0, space2) {
    float4 frustum_planes[6];
    float4 local_bounds_min;
    float4 local_bounds_max;
    row_major float4x4 current_view_projection;
    uint command_index;
    uint instance_base_offset;
    uint instance_count;
    uint hiz_mip_levels;
    float2 hiz_pyramid_size;
    float2 padding;
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
void main(uint3 dispatch_thread_id : SV_DispatchThreadID) {
    uint instance_index = dispatch_thread_id.x;
    if (instance_index >= instance_count) {
        return;
    }

    row_major float4x4 model = instance_models[instance_index];

    float3 corners[8] = {
        float3(local_bounds_min.x, local_bounds_min.y, local_bounds_min.z),
        float3(local_bounds_max.x, local_bounds_min.y, local_bounds_min.z),
        float3(local_bounds_min.x, local_bounds_max.y, local_bounds_min.z),
        float3(local_bounds_max.x, local_bounds_max.y, local_bounds_min.z),
        float3(local_bounds_min.x, local_bounds_min.y, local_bounds_max.z),
        float3(local_bounds_max.x, local_bounds_min.y, local_bounds_max.z),
        float3(local_bounds_min.x, local_bounds_max.y, local_bounds_max.z),
        float3(local_bounds_max.x, local_bounds_max.y, local_bounds_max.z),
    };

    float3 world_corners[8];
    for (int i = 0; i < 8; ++i) {
        world_corners[i] = mul(float4(corners[i], 1.0), model).xyz;
    }

    float3 world_min = world_corners[0];
    float3 world_max = world_corners[0];
    for (int i = 1; i < 8; ++i) {
        world_min = min(world_min, world_corners[i]);
        world_max = max(world_max, world_corners[i]);
    }

    if (!aabb_in_frustum(world_min, world_max)) {
        return;
    }

    if (hiz_mip_levels > 0) {
        // Reproject the same 8 world corners with THIS frame's camera to
        // get the object's actual current screen-space bounding rect (a
        // single point - even the object's own nearest corner - is not a
        // valid occlusion proxy in a dense scene: that one sampled pixel
        // can legitimately belong to a different, nearby object, causing a
        // false occlusion that has nothing to do with the object's own
        // visibility). Testing the whole rect and taking the max stored
        // depth over it is the conservative, correct test: only cull if
        // the object's nearest point is farther than EVERYTHING stored
        // across its whole footprint.
        float3 screen_min = float3(1e30, 1e30, 1e30);
        float3 screen_max = float3(-1e30, -1e30, -1e30);
        bool behind_camera = false;
        for (int j = 0; j < 8; ++j) {
            float4 clip = mul(float4(world_corners[j], 1.0), current_view_projection);
            if (clip.w <= 0.0) {
                behind_camera = true;
                break;
            }
            float3 ndc = clip.xyz / clip.w;
            screen_min = min(screen_min, ndc);
            screen_max = max(screen_max, ndc);
        }

        // clip.w <= 0 means a corner is behind the camera (e.g. an object
        // straddling the near plane) - reprojection is unreliable there, so
        // conservatively treat the instance as visible rather than risk a
        // false occlusion cull.
        if (!behind_camera) {
            // NDC xy in [-1,1] -> UV in [0,1] (uv.y=0 at the top, matching
            // fullscreen_vert.hlsl's own inverse mapping).
            float2 uv_min = float2(screen_min.x * 0.5 + 0.5, 0.5 - screen_max.y * 0.5);
            float2 uv_max = float2(screen_max.x * 0.5 + 0.5, 0.5 - screen_min.y * 0.5);

            float2 rect_texels = (uv_max - uv_min) * hiz_pyramid_size;
            float mip = clamp(
                ceil(log2(max(rect_texels.x, rect_texels.y))), 0.0,
                float(hiz_mip_levels - 1)
            );

            // Sample all 4 corners of the rect (not just one point) and
            // take the max. The chosen mip's texel size is >= the rect's
            // screen size, so the four corner texels jointly cover the
            // whole rect regardless of grid alignment - this is what
            // guarantees we're testing against everything actually stored
            // across the object's own footprint, not an arbitrary
            // unrelated neighboring pixel.
            float stored_depth = max(
                max(
                    hiz_pyramid.SampleLevel(hiz_sampler, float2(uv_min.x, uv_min.y), mip).r,
                    hiz_pyramid.SampleLevel(hiz_sampler, float2(uv_max.x, uv_min.y), mip).r
                ),
                max(
                    hiz_pyramid.SampleLevel(hiz_sampler, float2(uv_min.x, uv_max.y), mip).r,
                    hiz_pyramid.SampleLevel(hiz_sampler, float2(uv_max.x, uv_max.y), mip).r
                )
            );

            // Bias absorbs floating-point noise between the hardware
            // rasterizer's depth write (during the actual render) and this
            // shader's independent software reprojection of the same
            // point - without it, an object sitting exactly on its own
            // previously-written depth is a coin-flip self-comparison and
            // flickers (classic "depth acne", same class of issue shadow
            // mapping guards against with a bias).
            const float hiz_depth_bias = 0.0005;
            if (screen_min.z > stored_depth + hiz_depth_bias) {
                return;  // occluded
            }
        }
    }

    uint dest_slot;
    InterlockedAdd(indirect_commands[command_index].num_instances, 1, dest_slot);
    visible_instance_indices[instance_base_offset + dest_slot] = instance_index;
}
