// Clustered Forward+ light culling, pass 1 of 3 (count -> scan -> compact).
// One thread per cluster: tests every point/spot light's culling sphere
// against the cluster's view-space AABB (from cluster_aabb_build.hlsl) and
// writes how many of each matched. cluster_light_scan.hlsl turns these
// counts into offsets, then cluster_light_compact.hlsl re-runs the exact
// same test to write the matching light indices - both files must therefore
// keep the light_cull_radius/sphere_intersects_aabb functions and iteration
// order identical (mirrors the existing pbr_frag.hlsl/pbr_frag_alpha_test.hlsl
// duplication convention in this project).
//
// SDL_GPU compute HLSL register convention: space0 = read-only t/s,
// space1 = read-write u, space2 = uniform b.

struct PointLight {
    float4 position;
    float4 color;
};

struct SpotLight {
    float4 position;
    float4 direction;
    float3 color;
    float cut_off;
    float outer_cut_off;
    float3 padding;
};

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

StructuredBuffer<PointLight> point_lights : register(t0, space0);
StructuredBuffer<SpotLight> spot_lights : register(t1, space0);

// Written by cluster_aabb_build.hlsl; only read here, but bound via the
// read-write mechanism since it was created with compute-write usage.
RWStructuredBuffer<ClusterAABB> cluster_aabbs : register(u0, space1);
RWStructuredBuffer<ClusterLightGrid> cluster_light_grid : register(u1, space1);

// x = point_light_count, y = spot_light_count, z = total_clusters, w = unused.
cbuffer ClusterCullParams : register(b0, space2) {
    row_major float4x4 view_matrix;
    uint4 counts;
};

float light_cull_radius(float3 color) {
    const float cutoff = 1.0 / 16.0;
    float intensity = max(color.r, max(color.g, color.b));
    return sqrt(max(intensity, 0.0) / cutoff);
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
        float3 view_position = mul(float4(point_lights[i].position.xyz, 1.0), view_matrix).xyz;
        float radius = light_cull_radius(point_lights[i].color.rgb);
        if (sphere_intersects_aabb(view_position, radius, aabb_min, aabb_max)) {
            matched_point_count++;
        }
    }

    uint matched_spot_count = 0;
    for (uint i = 0; i < spot_light_count; ++i) {
        float3 view_position = mul(float4(spot_lights[i].position.xyz, 1.0), view_matrix).xyz;
        float radius = light_cull_radius(spot_lights[i].color);
        if (sphere_intersects_aabb(view_position, radius, aabb_min, aabb_max)) {
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
