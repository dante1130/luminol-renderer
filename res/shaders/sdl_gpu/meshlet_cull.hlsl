// Phase B of meshlet-level culling for the main color pass: one thread per
// (surviving Phase-A instance, candidate meshlet) pair. Phase A
// (SDL_GPUInstanceCullPass / instance_cull.hlsl) is untouched and runs
// first - it already does per-instance LOD selection + frustum/occlusion
// culling and compacts survivors per (submesh, LOD) into its own
// visible_instance_indices + IndirectDrawCommand buffers. This shader reads
// that output read-only and adds a finer culling pass: for each surviving
// instance, test each of its selected LOD's ~64-triangle meshlets
// (bounding sphere, transformed by that instance's model matrix) against
// the frustum and Hi-Z, and compact surviving (instance, meshlet) pairs
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
// Only bounds_center/bounds_radius are read here (local-space, transformed
// by the candidate instance's model matrix below); vertex_offset/
// triangle_offset/vertex_count/triangle_count are for the vertex shader
// (pbr_vert_meshlet.hlsl), not this cull pass.
struct GpuMeshletMetadata {
    uint vertex_offset;
    uint triangle_offset;
    uint vertex_count;
    uint triangle_count;
    float3 bounds_center;
    float bounds_radius;
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
// camera_world_position: used only by the occlusion test below to offset
// each candidate meshlet's bounding-sphere center toward the camera by its
// radius before sampling Hi-Z - paired with a trailing uint _padding to keep
// the cbuffer's total size a 16-byte multiple, the same pairing pattern
// instance_cull.hlsl's InstanceCullParams uses for lod_reference_position +
// enable_lod.
cbuffer MeshletCullParams : register(b0, space2) {
    float4 frustum_planes[6];
    row_major float4x4 current_view_projection;
    uint hiz_mip_levels;
    uint group_to_meshlet_dispatch_base;
    float2 hiz_pyramid_size;
    float3 camera_world_position;
    uint _padding;
};

bool sphere_in_frustum(float3 center, float radius) {
    for (uint i = 0; i < 6; ++i) {
        float4 plane = frustum_planes[i];
        if (dot(plane.xyz, center) + plane.w < -radius) {
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

    // Conservative (never-under-culls) uniform-scale approximation for a
    // sphere radius under a possibly non-uniformly-scaled model matrix -
    // take the largest of the three basis column lengths.
    float3 scale = float3(
        length(model[0].xyz), length(model[1].xyz), length(model[2].xyz)
    );
    float max_scale = max(scale.x, max(scale.y, scale.z));

    float3 world_center = mul(float4(meshlet.bounds_center, 1.0), model).xyz;
    float world_radius = meshlet.bounds_radius * max_scale;

    if (!sphere_in_frustum(world_center, world_radius)) {
        return;
    }

    if (hiz_mip_levels > 0) {
        // Single-point (not Phase A's 26-sample) occlusion test: a
        // deliberate simplification given this dispatch's much higher
        // thread count (worst case, one thread per candidate meshlet per
        // instance) - conservative correctness is traded for throughput
        // here, unlike Phase A's per-instance AABB test. Testing
        // world_center directly would under-estimate visibility: in NDC-z
        // space the center sits behind the sphere's near-facing surface by
        // ~world_radius, so a meshlet whose front hemisphere is actually
        // visible past an occluder but whose center projects behind it
        // would be wrongly culled. Offset to the near point on the sphere's
        // surface instead - the single-sample analogue of Phase A's
        // nearest-of-26-points test.
        float3 view_dir = normalize(camera_world_position - world_center);
        float3 near_point = world_center + (view_dir * world_radius);

        float4 clip = mul(float4(near_point, 1.0), current_view_projection);
        if (clip.w > 0.0) {
            float3 ndc = clip.xyz / clip.w;
            float2 uv = float2(ndc.x * 0.5 + 0.5, 0.5 - ndc.y * 0.5);
            float stored_depth = hiz_pyramid.SampleLevel(hiz_sampler, uv, 0.0).r;

            // Larger bias than Phase A's 0.0005: a single near-point sample
            // (vs. 26 surface samples) is a coarser approximation of the
            // meshlet's true silhouette, so more slack is needed to avoid
            // false-positive occlusion at grazing angles.
            const float hiz_depth_bias = 0.0015;
            if (ndc.z > stored_depth + hiz_depth_bias) {
                return;  // occluded
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
