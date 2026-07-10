#pragma once

#include <cstdint>

#include <LuminolMaths/Matrix.hpp>

#include <LuminolRenderEngine/Graphics/Light.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUComputePipeline.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTransferBuffer.hpp>

namespace Luminol::Graphics::SDL_GPU {

class GPUDevice;
class CommandBuffer;

// 16x9x24 froxel grid = 3456 clusters. Z is sliced exponentially (Doom 2016
// style) so slice thickness scales with distance from the camera, matching
// perspective depth precision.
constexpr auto cluster_grid_x = uint32_t{16};
constexpr auto cluster_grid_y = uint32_t{9};
constexpr auto cluster_grid_z = uint32_t{24};
constexpr auto cluster_count = cluster_grid_x * cluster_grid_y * cluster_grid_z;

// No cluster can match more lights than exist in the scene, so sizing the
// global index list to cluster_count * max_lights_per_cluster can never
// overflow (see cluster_light_scan.hlsl).
constexpr auto max_lights_per_cluster = max_point_lights + max_spot_lights;
constexpr auto global_light_index_capacity = cluster_count * max_lights_per_cluster;

// Owns the Clustered Forward+ cluster AABB grid and per-cluster light index
// lists (built via a 3-pass count -> scan -> compact compute pipeline).
class SDL_GPUClusterPass {
public:
    SDL_GPUClusterPass(GPUDevice& device);

    // Rebuilds the cluster AABB grid via a compute pass, but only if the
    // projection parameters differ from the last build (dirty-checked), so
    // a static camera projection doesn't pay the compute cost every frame.
    // Must be called while no render/copy pass on command_buffer is open.
    auto build_cluster_grid(
        CommandBuffer& command_buffer,
        float vertical_fov_degrees,
        float aspect_ratio,
        float near_plane,
        float far_plane
    ) -> void;

    // Uploads the current point/spot lights and culls them against every
    // cluster, producing get_cluster_light_grid_buffer() and
    // get_global_light_index_list_buffer(). Must be called after
    // build_cluster_grid() on the same command buffer (or a later one), and
    // while no render/copy pass on command_buffer is open.
    auto cull_lights(
        CommandBuffer& command_buffer,
        const Light& light_data,
        const Maths::Matrix4x4f& view_matrix
    ) -> void;

    [[nodiscard]] auto get_cluster_aabb_buffer() const -> const Buffer&;
    [[nodiscard]] auto get_cluster_light_grid_buffer() const -> const Buffer&;
    [[nodiscard]] auto get_global_light_index_list_buffer() const
        -> const Buffer&;
    [[nodiscard]] auto get_point_light_buffer() const -> const Buffer&;
    [[nodiscard]] auto get_spot_light_buffer() const -> const Buffer&;

private:
    ComputePipeline aabb_build_pipeline;
    ComputePipeline light_count_pipeline;
    ComputePipeline light_scan_pipeline;
    ComputePipeline light_compact_pipeline;

    Buffer cluster_aabb_buffer;
    Buffer cluster_light_grid_buffer;
    Buffer global_light_index_list_buffer;

    Buffer point_light_buffer;
    Buffer spot_light_buffer;
    TransferBuffer point_light_transfer_buffer;
    TransferBuffer spot_light_transfer_buffer;

    bool has_built = false;
    float cached_fov_degrees = 0.0F;
    float cached_aspect_ratio = 0.0F;
    float cached_near_plane = 0.0F;
    float cached_far_plane = 0.0F;
};

}  // namespace Luminol::Graphics::SDL_GPU
