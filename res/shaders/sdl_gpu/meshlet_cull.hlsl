// Phase B of meshlet-level culling for the main color pass: one thread per
// (surviving Phase-A instance, candidate meshlet) pair. Phase A
// (SDL_GPUInstanceCullPass / instance_cull.hlsl) is untouched and runs
// first - it already does per-instance LOD selection + frustum/occlusion
// culling and compacts survivors per (submesh, LOD) into its own
// visible_instance_indices + IndirectDrawCommand buffers. This shader reads
// that output read-only and adds a finer culling pass: for each surviving
// instance, test each of its selected LOD's ~64-triangle meshlets
// (local-space bounding box, transformed by that instance's model matrix)
// against the frustum and Hi-Z, and compact surviving (instance, meshlet) pairs
// into a new buffer for the main pass's vertex-pull draw
// (pbr_vert_meshlet.hlsl).
//
// Dispatch is sized on the CPU-known worst case (batch.instance_count x
// meshlets-in-that-LOD) with GPU-side early-exit against Phase A's actual
// surviving instance count - the same pattern instance_cull.hlsl already
// uses for its own worst-case-sized dispatch, not a new mechanism. This
// keeps output draw COUNT unchanged (still one non-indexed indirect draw
// per (submesh, LOD), matching Phase A's granularity) - see
// SDL_GPUMeshletCullPass.cpp for why a naive one-command-per-meshlet design
// was rejected (SDL_GPU's optional multiDrawIndirect Vulkan feature and the
// resulting draw-count explosion risk).
//
// SDL_GPU compute HLSL register convention: space0 = read-only t/s,
// space1 = read-write u, space2 = uniform b.

// Must match SDL_GPUMesh.cpp's meshlet_max_triangles.
#define MESHLET_MAX_TRIANGLES 64

// Cap on the exhaustive per-texel Hi-Z scan in main()'s occlusion test, in
// each screen axis independently (worst case 16*16 = 256 texel reads by one
// thread). Exceeding this resolves to "assume visible" rather than a partial
// scan - see the cap-exceeded bailout for why.
#define HIZ_RECT_MAX_TEXELS_PER_AXIS 16

// Non-indexed indirect draw command - mirrors SDL_GPUIndirectDrawCommand
// (SDL_gpu.h) exactly. 16 bytes - already a multiple of the 16-byte
// StructuredBuffer element stride, no padding needed.
struct IndirectDrawCommand {
    uint num_vertices;
    uint num_instances;
    uint first_vertex;
    uint first_instance;
};

// Mirrors SDL_GPUInstanceCullPass.cpp's IndirectDrawCommand (indexed
// variant - Phase A still draws indexed geometry for every other pass).
// Only num_instances is read here (Phase A's actual per-(submesh,LOD)
// survivor count); the other fields are irrelevant to Phase B.
struct PhaseAIndirectDrawCommand {
    uint num_indices;
    uint num_instances;
    uint first_index;
    int vertex_offset;
    uint first_instance;
};

// One (submesh, LOD) pair's Phase B dispatch inputs, built once per cull()
// call. All 8 fields are uint, 32 bytes total - already a multiple of the
// 16-byte StructuredBuffer element stride (see
// SDL_GPUInstanceCullPass.cpp's SubmeshCullMetadata for why that multiple
// matters), no padding field needed.
struct MeshletCullMetadata {
    // Index into phase_a_commands - read to get this (submesh,LOD)'s actual
    // surviving instance count (Phase A's num_instances, GPU-written).
    uint phase_a_command_index;
    // Base offset into phase_a_visible_instance_indices for this
    // (submesh,LOD) - mirrors Phase A's own instance_base_offset.
    uint phase_a_instance_base;
    // This (submesh,LOD)'s slice of the renderable's shared meshlet arrays
    // (see GpuMeshletMetadata, SDL_GPUMesh::get_meshlet_range).
    uint meshlet_first;
    uint meshlet_count;
    // CPU-known upper bound on surviving instances (== batch.instance_count)
    // - worst-case dispatch sizing, not the actual (GPU-known) count.
    uint worst_case_instance_count;
    // Index into output_commands - this (submesh,LOD)'s own non-indexed
    // draw command, incremented by surviving meshlet-instances below.
    uint output_command_index;
    // Base offset into visible_meshlet_instances for this (submesh,LOD).
    uint output_instance_base;
    // This (submesh,LOD)'s starting thread-group index within the batch's
    // dispatch (mirrors Phase A's first_group / group_to_submesh pattern).
    uint first_group;
};

// One meshlet cluster's data, shared across every instance/LOD that
// references it - mirrors GpuMeshletMetadata (SDL_GPUMesh.hpp) exactly.
// local_bounds_min/max (local-space, transformed by the candidate
// instance's model matrix below) are what this cull pass actually tests
// against - see SDL_GPUMesh.hpp's doc comment on GpuMeshletMetadata for why
// an AABB replaced the bounding sphere here (thin/elongated meshlets - a
// curtain fold, a column edge, foliage - have a sphere radius much larger
// than their true cross-section, letting sphere-surface sample points land
// in occluded space next to real, visible, thinner geometry).
// bounds_center/bounds_radius are kept (unused by this shader) only because
// meshopt_computeMeshletBounds already computes them for free and cone data
// may use them later (see build_meshlets' cone-weight comment).
// vertex_offset/triangle_offset/vertex_count/triangle_count are for the
// vertex shader (pbr_vert_meshlet.hlsl), not this cull pass.
struct GpuMeshletMetadata {
    uint vertex_offset;
    uint triangle_offset;
    uint vertex_count;
    uint triangle_count;
    float3 bounds_center;
    float bounds_radius;
    float4 local_bounds_min;
    float4 local_bounds_max;
};

// hiz_pyramid/hiz_sampler share t0/s0 to compile as one combined-image-
// sampler descriptor, mirroring instance_cull.hlsl.
Texture2D hiz_pyramid : register(t0, space0);
SamplerState hiz_sampler : register(s0, space0);
StructuredBuffer<row_major float4x4> instance_models : register(t1, space0);
StructuredBuffer<uint> phase_a_visible_instance_indices : register(t2, space0);
StructuredBuffer<PhaseAIndirectDrawCommand> phase_a_commands : register(t3, space0);
StructuredBuffer<MeshletCullMetadata> meshlet_cull_metadata : register(t4, space0);
StructuredBuffer<GpuMeshletMetadata> mesh_meshlet_metadata : register(t5, space0);
// One entry per thread group in this dispatch, indexed by
// group_to_meshlet_dispatch_base + SV_GroupID.x: which meshlet_cull_metadata
// entry that group belongs to - mirrors instance_cull.hlsl's
// group_to_submesh.
StructuredBuffer<uint> group_to_meshlet_dispatch : register(t6, space0);

RWStructuredBuffer<IndirectDrawCommand> output_commands : register(u0, space1);
// x = original instance index (into instance_models), y = meshlet index
// (into mesh_meshlet_metadata) - a plain uint2, not a wrapped struct, so
// its StructuredBuffer element stride is unambiguously 8 bytes on every
// backend (see the comment on MeshletCullMetadata's 16-byte-multiple sizing
// for why this project no longer takes struct/vector layout on faith).
RWStructuredBuffer<uint2> visible_meshlet_instances : register(u1, space1);

// frustum_planes/current_view_projection/hiz_mip_levels/hiz_pyramid_size:
// same meaning as instance_cull.hlsl's InstanceCullParams. No LOD fields
// here - LOD selection already happened in Phase A; this pass only culls
// the LOD each surviving instance already picked, at meshlet granularity.
cbuffer MeshletCullParams : register(b0, space2) {
    float4 frustum_planes[6];
    row_major float4x4 current_view_projection;
    uint hiz_mip_levels;
    uint group_to_meshlet_dispatch_base;
    float2 hiz_pyramid_size;
};

// Mirrors instance_cull.hlsl's aabb_in_frustum exactly (same scale-invariant
// sign test - immune to frustum_planes being unnormalized, unlike a sphere
// test would be).
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
    uint global_group_index = group_to_meshlet_dispatch_base + group_id.x;
    uint metadata_index = group_to_meshlet_dispatch[global_group_index];
    MeshletCullMetadata metadata = meshlet_cull_metadata[metadata_index];

    uint local_group_index = global_group_index - metadata.first_group;
    uint local_thread_index = (local_group_index * 64) + group_thread_id.x;

    // Decode (instance_slot, meshlet_slot) from the linear thread index -
    // instance-major, meshlet-minor (see MeshletCullMetadata.meshlet_count).
    uint instance_slot = local_thread_index / metadata.meshlet_count;
    uint meshlet_slot = local_thread_index % metadata.meshlet_count;

    if (instance_slot >= metadata.worst_case_instance_count) {
        return;
    }

    // Phase A's actual surviving instance count for this (submesh,LOD) is
    // only known on the GPU (written via InterlockedAdd during its own
    // dispatch) - the worst-case check above only bounds the CPU-sized
    // dispatch itself.
    uint actual_instance_count =
        phase_a_commands[metadata.phase_a_command_index].num_instances;
    if (instance_slot >= actual_instance_count) {
        return;
    }

    uint original_instance_index = phase_a_visible_instance_indices[
        metadata.phase_a_instance_base + instance_slot
    ];
    row_major float4x4 model = instance_models[original_instance_index];

    uint meshlet_index = metadata.meshlet_first + meshlet_slot;
    GpuMeshletMetadata meshlet = mesh_meshlet_metadata[meshlet_index];

    float3 local_bounds_min = meshlet.local_bounds_min.xyz;
    float3 local_bounds_max = meshlet.local_bounds_max.xyz;

    // Only the 8 true AABB corners are needed - view-space Z is linear, so
    // its minimum over a convex box occurs at a vertex (giving the exact
    // nearest depth, not an approximation), and a convex box's projected
    // screen footprint's convex hull equals the hull of its projected
    // vertices (given none are behind the camera, guarded by the
    // behind_camera bailout below) - so these 8 corners exactly bound both
    // the frustum test and (see below) the occlusion test's screen rect. No
    // face-center/edge-midpoint samples needed - those only existed to feed
    // point sampling, which this occlusion test no longer does (see below).
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
        // Exact screen-space rectangle coverage, not point sampling: earlier
        // rounds went from 1 -> 5 -> 9 -> 26 discrete sample points, but no
        // fixed point count can guarantee catching every possible thin,
        // unluckily-angled visible sliver between occluders - a gap can
        // always fall between samples. Instead, reproject the 8 corners to
        // get the meshlet's exact nearest depth and exact screen-space UV
        // rect (both provably vertex-extremal for a convex box - see the
        // comment above), then scan EVERY Hi-Z texel in that rect below, so
        // no pixel of the footprint goes unchecked.
        float nearest_ndc_z = 1e30;
        float2 uv_min = float2(1e30, 1e30);
        float2 uv_max = float2(-1e30, -1e30);
        bool behind_camera = false;
        for (int j = 0; j < 8; ++j) {
            float4 clip = mul(
                float4(world_corners[j], 1.0), current_view_projection
            );
            if (clip.w <= 0.0) {
                behind_camera = true;
                break;
            }
            float3 ndc = clip.xyz / clip.w;
            nearest_ndc_z = min(nearest_ndc_z, ndc.z);
            float2 uv = float2(ndc.x * 0.5 + 0.5, 0.5 - ndc.y * 0.5);
            uv_min = min(uv_min, uv);
            uv_max = max(uv_max, uv);
        }

        if (!behind_camera) {
            // A corner can legitimately project outside [0,1] for a meshlet
            // straddling the frustum edge (aabb_in_frustum above only
            // rejects it if EVERY plane fully excludes the box) - clamp to
            // the visible screen before converting to texel coordinates.
            uv_min = saturate(uv_min);
            uv_max = saturate(uv_max);

            int2 pyramid_size_i = int2(hiz_pyramid_size);
            int2 texel_min = clamp(
                int2(floor(uv_min * hiz_pyramid_size)), int2(0, 0),
                pyramid_size_i - int2(1, 1)
            );
            int2 texel_max = clamp(
                int2(floor(uv_max * hiz_pyramid_size)), int2(0, 0),
                pyramid_size_i - int2(1, 1)
            );
            int2 rect_texels = texel_max - texel_min + int2(1, 1);

            // Capped so one thread can't stall its warp/wavefront on an
            // unbounded, data-dependent loop for a meshlet very close to the
            // camera. Past the cap, skip the test and assume visible rather
            // than cull from a partial scan - a fully-occluded CHECKED
            // subset can't soundly prove the whole footprint occluded, so
            // only skipping the test entirely stays conservative (same
            // policy as the behind_camera bailout above).
            if (rect_texels.x <= HIZ_RECT_MAX_TEXELS_PER_AXIS &&
                rect_texels.y <= HIZ_RECT_MAX_TEXELS_PER_AXIS) {
                float stored_depth = -1e30;
                [loop]
                for (int y = texel_min.y; y <= texel_max.y; ++y) {
                    [loop]
                    for (int x = texel_min.x; x <= texel_max.x; ++x) {
                        // Sample (not Load) at each texel's exact center via
                        // the existing Nearest/ClampToEdge hiz_sampler - this
                        // file's hiz_pyramid/hiz_sampler must be used
                        // together to compile as one combined-image-sampler
                        // descriptor (see the register-binding comment
                        // above), so this keeps the same compile path
                        // functionally equivalent to Load given Nearest
                        // filtering.
                        float2 texel_uv =
                            (float2(x, y) + 0.5) / hiz_pyramid_size;
                        stored_depth = max(
                            stored_depth,
                            hiz_pyramid.SampleLevel(hiz_sampler, texel_uv, 0.0).r
                        );
                    }
                }

                // Same bias as before: absorbs floating-point noise between
                // the rasterizer's depth write and this shader's independent
                // reprojection of the same geometry.
                const float hiz_depth_bias = 0.0015;
                if (nearest_ndc_z > stored_depth + hiz_depth_bias) {
                    return;  // occluded
                }
            }
        }
    }

    uint dest_slot;
    InterlockedAdd(
        output_commands[metadata.output_command_index].num_instances, 1,
        dest_slot
    );
    visible_meshlet_instances[metadata.output_instance_base + dest_slot] =
        uint2(original_instance_index, meshlet_index);
}
