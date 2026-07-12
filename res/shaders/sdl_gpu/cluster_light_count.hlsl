// Clustered Forward+ light culling, pass 1 of 3 (count -> scan -> compact).
// One thread per cluster: tests every point/spot light's culling sphere
// against the cluster's view-space AABB (from cluster_aabb_build.hlsl) and
// writes how many of each matched. cluster_light_scan.hlsl turns these
// counts into offsets, then cluster_light_compact.hlsl re-runs the exact
// same test to write the matching light indices - both files must therefore
// keep the sphere_intersects_aabb function and iteration order identical
// (mirrors the existing pbr_frag.hlsl/pbr_frag_alpha_test.hlsl duplication
// convention in this project).
//
// Light view-space position + cull radius are precomputed once per frame on
// the CPU (SDL_GPUClusterPass::cull_lights) rather than recomputed per
// cluster here, since neither depends on the cluster being tested - see
// point_light_culling_data/spot_light_culling_data.
//
// SDL_GPU compute HLSL register convention: space0 = read-only t/s,
// space1 = read-write u, space2 = uniform b.

struct ClusterAABB {
    float4 min_view;
    float4 max_view;
};

struct ClusterLightGrid {
    uint offset;
    uint point_count;
    uint spot_count;
    uint padding;
};

// xyz = view-space position, w = cull radius.
StructuredBuffer<float4> point_light_culling_data : register(t0, space0);
StructuredBuffer<float4> spot_light_culling_data : register(t1, space0);

// Written by cluster_aabb_build.hlsl; only read here, but bound via the
// read-write mechanism since it was created with compute-write usage.
RWStructuredBuffer<ClusterAABB> cluster_aabbs : register(u0, space1);
RWStructuredBuffer<ClusterLightGrid> cluster_light_grid : register(u1, space1);

// x = point_light_count, y = spot_light_count, z = total_clusters, w = unused.
cbuffer ClusterCullParams : register(b0, space2) {
    uint4 counts;
}

bool sphere_intersects_aabb(float3 center, float radius, float3 box_min, float3 box_max) {
    float3 closest = clamp(center, box_min, box_max);
    float3 delta = center - closest;
    return dot(delta, delta) <= radius * radius;
}

[numthreads(64, 1, 1)]
void main(uint3 dispatch_thread_id : SV_DispatchThreadID) {
    uint cluster_index = dispatch_thread_id.x;
    uint total_clusters = counts.z;
    if (cluster_index >= total_clusters) {
        return;
    }

    float3 aabb_min = cluster_aabbs[cluster_index].min_view.xyz;
    float3 aabb_max = cluster_aabbs[cluster_index].max_view.xyz;

    uint point_light_count = counts.x;
    uint spot_light_count = counts.y;

    uint matched_point_count = 0;
    for (uint i = 0; i < point_light_count; ++i) {
        float4 culling_data = point_light_culling_data[i];
        if (sphere_intersects_aabb(culling_data.xyz, culling_data.w, aabb_min, aabb_max)) {
            matched_point_count++;
        }
    }

    uint matched_spot_count = 0;
    for (uint i = 0; i < spot_light_count; ++i) {
        float4 culling_data = spot_light_culling_data[i];
        if (sphere_intersects_aabb(culling_data.xyz, culling_data.w, aabb_min, aabb_max)) {
            matched_spot_count++;
        }
    }

    ClusterLightGrid grid;
    grid.offset = 0;
    grid.point_count = matched_point_count;
    grid.spot_count = matched_spot_count;
    grid.padding = 0;
    cluster_light_grid[cluster_index] = grid;
}
