// Per-instance frustum culling: one thread per instance. Transforms the
// submesh's local-space AABB by that instance's model matrix, tests it
// against the camera frustum (Gribb/Hartmann planes, see Frustum.cpp), and
// for survivors atomically appends the instance's original index into
// visible_instance_indices and increments the corresponding
// IndirectDrawCommand's num_instances. This replaces an O(instance count)
// CPU loop (SDL_GPUCullingUtils::compute_batch_mesh_world_bounds) with one
// GPU dispatch per submesh, so cost no longer scales with instance count on
// the CPU (see SDL_GPUInstanceCullPass).
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

StructuredBuffer<row_major float4x4> instance_models : register(t0, space0);

RWStructuredBuffer<IndirectDrawCommand> indirect_commands : register(u0, space1);
RWStructuredBuffer<uint> visible_instance_indices : register(u1, space1);

// frustum_planes: left, right, bottom, top, near, far. local_bounds_min/max:
// this submesh's local-space AABB. command_index: this submesh's slot in
// indirect_commands. instance_base_offset: this submesh's slice base in
// visible_instance_indices. instance_count: this batch's instance count
// (thread count ceiling).
cbuffer InstanceCullParams : register(b0, space2) {
    float4 frustum_planes[6];
    float4 local_bounds_min;
    float4 local_bounds_max;
    uint command_index;
    uint instance_base_offset;
    uint instance_count;
    uint padding;
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

    float3 world_min = mul(float4(corners[0], 1.0), model).xyz;
    float3 world_max = world_min;
    for (int i = 1; i < 8; ++i) {
        float3 world_corner = mul(float4(corners[i], 1.0), model).xyz;
        world_min = min(world_min, world_corner);
        world_max = max(world_max, world_corner);
    }

    if (!aabb_in_frustum(world_min, world_max)) {
        return;
    }

    uint dest_slot;
    InterlockedAdd(indirect_commands[command_index].num_instances, 1, dest_slot);
    visible_instance_indices[instance_base_offset + dest_slot] = instance_index;
}
