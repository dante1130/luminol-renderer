#pragma once

#include <cstdint>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUComputePipeline.hpp>

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

// Owns the Clustered Forward+ cluster AABB grid and (in a later phase) the
// per-cluster light index lists. Currently only builds the AABB grid.
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

    [[nodiscard]] auto get_cluster_aabb_buffer() const -> const Buffer&;

private:
    ComputePipeline aabb_build_pipeline;
    Buffer cluster_aabb_buffer;

    bool has_built = false;
    float cached_fov_degrees = 0.0F;
    float cached_aspect_ratio = 0.0F;
    float cached_near_plane = 0.0F;
    float cached_far_plane = 0.0F;
};

}  // namespace Luminol::Graphics::SDL_GPU
