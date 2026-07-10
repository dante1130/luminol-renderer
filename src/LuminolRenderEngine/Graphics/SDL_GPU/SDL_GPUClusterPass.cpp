#include "SDL_GPUClusterPass.hpp"

#include <array>
#include <cmath>
#include <numbers>

#include <gsl/gsl>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCommandBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUComputePass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUDevice.hpp>

namespace {

using namespace Luminol::Graphics::SDL_GPU;

// float4 min_view + float4 max_view.
constexpr auto cluster_aabb_size_bytes = uint32_t{32};
constexpr auto cluster_aabb_buffer_size = cluster_count * cluster_aabb_size_bytes;
constexpr auto threads_per_group = uint32_t{64};

// Mirrors cbuffer ClusterBuildParams in cluster_aabb_build.hlsl.
struct ClusterBuildParams {
    std::array<float, 4> camera_params;
    std::array<uint32_t, 4> grid_dim;
};

auto make_aabb_build_pipeline(GPUDevice& device) -> ComputePipeline {
    return device.create_compute_pipeline(ComputePipelineInfo{
        .path = "res/shaders/sdl_gpu/cluster_aabb_build.hlsl",
        .source_language = ShaderSourceLanguage::Hlsl,
        .readwrite_storage_buffer_count = 1,
        .uniform_buffer_count = 1,
        .threadcount_x = threads_per_group,
        .threadcount_y = 1,
        .threadcount_z = 1,
    });
}

}  // namespace

namespace Luminol::Graphics::SDL_GPU {

SDL_GPUClusterPass::SDL_GPUClusterPass(GPUDevice& device)
    : aabb_build_pipeline{make_aabb_build_pipeline(device)},
      cluster_aabb_buffer{device.create_buffer(BufferInfo{
          .usage = BufferUsage::ComputeStorageReadWrite,
          .size = cluster_aabb_buffer_size,
      })} {}

auto SDL_GPUClusterPass::build_cluster_grid(
    CommandBuffer& command_buffer,
    float vertical_fov_degrees,
    float aspect_ratio,
    float near_plane,
    float far_plane
) -> void {
    const auto unchanged = has_built &&
        cached_fov_degrees == vertical_fov_degrees &&
        cached_aspect_ratio == aspect_ratio &&
        cached_near_plane == near_plane && cached_far_plane == far_plane;
    if (unchanged) {
        return;
    }

    const auto tan_half_fov_y = std::tan(
        vertical_fov_degrees *
        (std::numbers::pi_v<float> / 180.0F) / 2.0F
    );

    const auto params = ClusterBuildParams{
        .camera_params = {tan_half_fov_y, aspect_ratio, near_plane, far_plane},
        .grid_dim = {cluster_grid_x, cluster_grid_y, cluster_grid_z, 0},
    };

    const auto storage_bindings = std::array<StorageBufferReadWriteBinding, 1>{
        StorageBufferReadWriteBinding{
            .buffer = &cluster_aabb_buffer, .cycle = false
        }
    };
    auto compute_pass = command_buffer.begin_compute_pass(storage_bindings);
    command_buffer.push_compute_uniform_data(
        0,
        gsl::span<const std::byte>{
            reinterpret_cast<const std::byte*>(&params), sizeof(params)
        }
    );
    compute_pass.bind_compute_pipeline(aabb_build_pipeline);
    compute_pass.dispatch(
        (cluster_count + threads_per_group - 1) / threads_per_group, 1, 1
    );

    has_built = true;
    cached_fov_degrees = vertical_fov_degrees;
    cached_aspect_ratio = aspect_ratio;
    cached_near_plane = near_plane;
    cached_far_plane = far_plane;
}

auto SDL_GPUClusterPass::get_cluster_aabb_buffer() const -> const Buffer& {
    return cluster_aabb_buffer;
}

}  // namespace Luminol::Graphics::SDL_GPU
