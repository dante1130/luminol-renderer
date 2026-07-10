// Clustered Forward+ light culling, pass 2 of 3 (count -> scan -> compact).
// Turns per-cluster point_count/spot_count (written by cluster_light_count.hlsl)
// into an exclusive prefix-sum offset into the global light index list. Only
// 3456 clusters (16x9x24 grid) so a single-thread serial scan is cheap and
// avoids the complexity of a parallel scan algorithm.
//
// The global light index list is sized cluster_count * (max_point_lights +
// max_spot_lights), which by construction can never be smaller than the sum
// of counts across all clusters (no cluster can match more lights than exist
// in the scene), so no overflow clamping is needed here.

struct ClusterLightGrid {
    uint offset;
    uint point_count;
    uint spot_count;
    uint padding;
};

RWStructuredBuffer<ClusterLightGrid> cluster_light_grid : register(u0, space1);

// x = total_clusters.
cbuffer ClusterScanParams : register(b0, space2) {
    uint4 params;
};

[numthreads(1, 1, 1)]
void main() {
    uint running_offset = 0;
    uint total_clusters = params.x;
    for (uint i = 0; i < total_clusters; ++i) {
        cluster_light_grid[i].offset = running_offset;
        running_offset += cluster_light_grid[i].point_count + cluster_light_grid[i].spot_count;
    }
}
