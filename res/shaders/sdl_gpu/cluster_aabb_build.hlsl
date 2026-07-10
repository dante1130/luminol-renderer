// Builds a view-space axis-aligned bounding box for every cluster in the
// Clustered Forward+ froxel grid. One thread per cluster; the grid is
// flattened to a 1D dispatch since the total cluster count need not be a
// clean multiple of a 3D threadgroup shape.
//
// Rebuilt only when the camera projection (fov/aspect/near/far) changes,
// not every frame - see SDL_GPUClusterPass::build_cluster_grid.
//
// SDL_GPU compute HLSL register convention (fixed by SDL_CreateGPUComputePipeline,
// distinct from the graphics-stage space2/space3 convention used elsewhere in
// this project): space0 = read-only t/s, space1 = read-write u, space2 = uniform b.

struct ClusterAABB {
    float4 min_view;
    float4 max_view;
};

RWStructuredBuffer<ClusterAABB> cluster_aabbs : register(u0, space1);

// x = tan(vertical_fov / 2), y = aspect_ratio, z = near_plane, w = far_plane.
// grid_dim: x = cluster count X, y = cluster count Y, z = cluster count Z
// (depth slices), w = unused.
cbuffer ClusterBuildParams : register(b0, space2) {
    float4 camera_params;
    uint4 grid_dim;
};

float3 cluster_corner_view_position(
    float x_ndc, float y_ndc, float z_view, float tan_half_fov_y, float aspect_ratio
) {
    float x_view = x_ndc * z_view * aspect_ratio * tan_half_fov_y;
    float y_view = y_ndc * z_view * tan_half_fov_y;
    return float3(x_view, y_view, z_view);
}

[numthreads(64, 1, 1)]
void main(uint3 dispatch_thread_id : SV_DispatchThreadID) {
    uint cluster_index = dispatch_thread_id.x;
    uint total_clusters = grid_dim.x * grid_dim.y * grid_dim.z;
    if (cluster_index >= total_clusters) {
        return;
    }

    uint tiles_per_slice = grid_dim.x * grid_dim.y;
    uint cz = cluster_index / tiles_per_slice;
    uint slice_remainder = cluster_index % tiles_per_slice;
    uint cy = slice_remainder / grid_dim.x;
    uint cx = slice_remainder % grid_dim.x;

    float tan_half_fov_y = camera_params.x;
    float aspect_ratio = camera_params.y;
    float near_plane = camera_params.z;
    float far_plane = camera_params.w;

    // Exponential depth slicing (Doom 2016 style) so slice thickness scales
    // with distance from the camera, matching perspective depth precision.
    float slice_ratio = far_plane / near_plane;
    float z_near_slice =
        near_plane * pow(slice_ratio, float(cz) / float(grid_dim.z));
    float z_far_slice =
        near_plane * pow(slice_ratio, float(cz + 1) / float(grid_dim.z));

    float x_ndc_min = -1.0 + 2.0 * float(cx) / float(grid_dim.x);
    float x_ndc_max = -1.0 + 2.0 * float(cx + 1) / float(grid_dim.x);
    float y_ndc_min = -1.0 + 2.0 * float(cy) / float(grid_dim.y);
    float y_ndc_max = -1.0 + 2.0 * float(cy + 1) / float(grid_dim.y);

    float3 corners[8] = {
        cluster_corner_view_position(x_ndc_min, y_ndc_min, z_near_slice, tan_half_fov_y, aspect_ratio),
        cluster_corner_view_position(x_ndc_max, y_ndc_min, z_near_slice, tan_half_fov_y, aspect_ratio),
        cluster_corner_view_position(x_ndc_min, y_ndc_max, z_near_slice, tan_half_fov_y, aspect_ratio),
        cluster_corner_view_position(x_ndc_max, y_ndc_max, z_near_slice, tan_half_fov_y, aspect_ratio),
        cluster_corner_view_position(x_ndc_min, y_ndc_min, z_far_slice, tan_half_fov_y, aspect_ratio),
        cluster_corner_view_position(x_ndc_max, y_ndc_min, z_far_slice, tan_half_fov_y, aspect_ratio),
        cluster_corner_view_position(x_ndc_min, y_ndc_max, z_far_slice, tan_half_fov_y, aspect_ratio),
        cluster_corner_view_position(x_ndc_max, y_ndc_max, z_far_slice, tan_half_fov_y, aspect_ratio),
    };

    float3 min_corner = corners[0];
    float3 max_corner = corners[0];
    for (int i = 1; i < 8; ++i) {
        min_corner = min(min_corner, corners[i]);
        max_corner = max(max_corner, corners[i]);
    }

    ClusterAABB result;
    result.min_view = float4(min_corner, 0.0);
    result.max_view = float4(max_corner, 0.0);
    cluster_aabbs[cluster_index] = result;
}
