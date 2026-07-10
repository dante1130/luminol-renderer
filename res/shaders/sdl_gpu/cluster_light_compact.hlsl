// Clustered Forward+ light culling, pass 3 of 3 (count -> scan -> compact).
// Re-runs the exact same sphere-vs-AABB test as cluster_light_count.hlsl (the
// two files must stay in sync) and writes matching light indices into the
// global index list at [offset, offset+point_count) for point lights and
// [offset+point_count, offset+point_count+spot_count) for spot lights.
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
// Offsets already computed by cluster_light_scan.hlsl; only read here.
RWStructuredBuffer<ClusterLightGrid> cluster_light_grid : register(u1, space1);
RWStructuredBuffer<uint> global_light_index_list : register(u2, space1);

// x = point_light_count, y = spot_light_count, z = total_clusters, w = unused.
cbuffer ClusterCullParams : register(b0, space2) {
    row_major float4x4 view_matrix;
    uint4 counts;
};

float light_cull_radius(float3 color) {
    const float cutoff = 1.0 / 256.0;
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

    uint offset = cluster_light_grid[cluster_index].offset;
    uint local_count = 0;

    uint point_light_count = counts.x;
    for (uint i = 0; i < point_light_count; ++i) {
        float3 view_position = mul(float4(point_lights[i].position.xyz, 1.0), view_matrix).xyz;
        float radius = light_cull_radius(point_lights[i].color.rgb);
        if (sphere_intersects_aabb(view_position, radius, aabb_min, aabb_max)) {
            global_light_index_list[offset + local_count] = i;
            local_count++;
        }
    }

    uint spot_light_count = counts.y;
    for (uint i = 0; i < spot_light_count; ++i) {
        float3 view_position = mul(float4(spot_lights[i].position.xyz, 1.0), view_matrix).xyz;
        float radius = light_cull_radius(spot_lights[i].color);
        if (sphere_intersects_aabb(view_position, radius, aabb_min, aabb_max)) {
            global_light_index_list[offset + local_count] = i;
            local_count++;
        }
    }
}
