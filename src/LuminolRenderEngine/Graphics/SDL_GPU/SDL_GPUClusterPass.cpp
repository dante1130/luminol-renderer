#include "SDL_GPUClusterPass.hpp"

#include <array>
#include <cmath>
#include <cstring>
#include <numbers>

#include <gsl/gsl>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCommandBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUComputePass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCopyPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUDevice.hpp>

namespace {

using namespace Luminol::Graphics;
using namespace Luminol::Graphics::SDL_GPU;

// float4 min_view + float4 max_view.
constexpr auto cluster_aabb_size_bytes = uint32_t{32};
constexpr auto cluster_aabb_buffer_size = cluster_count * cluster_aabb_size_bytes;
// uint offset + uint point_count + uint spot_count + uint padding.
constexpr auto cluster_light_grid_element_size = uint32_t{16};
constexpr auto cluster_light_grid_buffer_size =
    cluster_count * cluster_light_grid_element_size;
constexpr auto global_light_index_list_buffer_size =
    global_light_index_capacity * static_cast<uint32_t>(sizeof(uint32_t));
constexpr auto point_light_buffer_size =
    max_point_lights * static_cast<uint32_t>(sizeof(AlignedPointLight));
constexpr auto spot_light_buffer_size =
    max_spot_lights * static_cast<uint32_t>(sizeof(AlignedSpotLight));

constexpr auto threads_per_group = uint32_t{64};
constexpr auto dispatch_group_count =
    (cluster_count + threads_per_group - 1) / threads_per_group;

// Mirrors cbuffer ClusterBuildParams in cluster_aabb_build.hlsl.
struct ClusterBuildParams {
    std::array<float, 4> camera_params;
    std::array<uint32_t, 4> grid_dim;
};

// Mirrors cbuffer ClusterCullParams in cluster_light_count.hlsl and
// cluster_light_compact.hlsl.
struct ClusterCullParams {
    Luminol::Maths::Matrix4x4f view_matrix;
    std::array<uint32_t, 4> counts;
};

// Mirrors cbuffer ClusterScanParams in cluster_light_scan.hlsl.
struct ClusterScanParams {
    std::array<uint32_t, 4> params;
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

auto make_light_count_pipeline(GPUDevice& device) -> ComputePipeline {
    return device.create_compute_pipeline(ComputePipelineInfo{
        .path = "res/shaders/sdl_gpu/cluster_light_count.hlsl",
        .source_language = ShaderSourceLanguage::Hlsl,
        .readonly_storage_buffer_count = 2,
        .readwrite_storage_buffer_count = 2,
        .uniform_buffer_count = 1,
        .threadcount_x = threads_per_group,
        .threadcount_y = 1,
        .threadcount_z = 1,
    });
}

auto make_light_scan_pipeline(GPUDevice& device) -> ComputePipeline {
    return device.create_compute_pipeline(ComputePipelineInfo{
        .path = "res/shaders/sdl_gpu/cluster_light_scan.hlsl",
        .source_language = ShaderSourceLanguage::Hlsl,
        .readwrite_storage_buffer_count = 1,
        .uniform_buffer_count = 1,
        .threadcount_x = 1,
        .threadcount_y = 1,
        .threadcount_z = 1,
    });
}

auto make_light_compact_pipeline(GPUDevice& device) -> ComputePipeline {
    return device.create_compute_pipeline(ComputePipelineInfo{
        .path = "res/shaders/sdl_gpu/cluster_light_compact.hlsl",
        .source_language = ShaderSourceLanguage::Hlsl,
        .readonly_storage_buffer_count = 2,
        .readwrite_storage_buffer_count = 3,
        .uniform_buffer_count = 1,
        .threadcount_x = threads_per_group,
        .threadcount_y = 1,
        .threadcount_z = 1,
    });
}

template <typename T>
auto upload_lights(
    CopyPass& copy_pass,
    TransferBuffer& transfer_buffer,
    const Buffer& destination_buffer,
    gsl::span<const T> lights
) -> void {
    const auto size = static_cast<uint32_t>(lights.size() * sizeof(T));
    const auto mapped = transfer_buffer.map(true);
    std::memcpy(mapped.data(), lights.data(), size);
    transfer_buffer.unmap();

    copy_pass.upload_to_buffer(transfer_buffer, 0, destination_buffer, 0, size, true);
}

}  // namespace

namespace Luminol::Graphics::SDL_GPU {

SDL_GPUClusterPass::SDL_GPUClusterPass(GPUDevice& device)
    : aabb_build_pipeline{make_aabb_build_pipeline(device)},
      light_count_pipeline{make_light_count_pipeline(device)},
      light_scan_pipeline{make_light_scan_pipeline(device)},
      light_compact_pipeline{make_light_compact_pipeline(device)},
      cluster_aabb_buffer{device.create_buffer(BufferInfo{
          .usage = BufferUsage::ComputeStorageReadWrite,
          .size = cluster_aabb_buffer_size,
      })},
      cluster_light_grid_buffer{device.create_buffer(BufferInfo{
          .usage = BufferUsage::ComputeStorageReadWrite | BufferUsage::StorageRead,
          .size = cluster_light_grid_buffer_size,
      })},
      global_light_index_list_buffer{device.create_buffer(BufferInfo{
          .usage = BufferUsage::ComputeStorageReadWrite | BufferUsage::StorageRead,
          .size = global_light_index_list_buffer_size,
      })},
      point_light_buffer{device.create_buffer(BufferInfo{
          .usage = BufferUsage::ComputeStorageRead | BufferUsage::StorageRead,
          .size = point_light_buffer_size,
      })},
      spot_light_buffer{device.create_buffer(BufferInfo{
          .usage = BufferUsage::ComputeStorageRead | BufferUsage::StorageRead,
          .size = spot_light_buffer_size,
      })},
      point_light_transfer_buffer{device.create_transfer_buffer(TransferBufferInfo{
          .usage = TransferBufferUsage::Upload,
          .size = point_light_buffer_size,
      })},
      spot_light_transfer_buffer{device.create_transfer_buffer(TransferBufferInfo{
          .usage = TransferBufferUsage::Upload,
          .size = spot_light_buffer_size,
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
    auto compute_pass = command_buffer.begin_compute_pass({}, storage_bindings);
    command_buffer.push_compute_uniform_data(
        0,
        gsl::span<const std::byte>{
            reinterpret_cast<const std::byte*>(&params), sizeof(params)
        }
    );
    compute_pass.bind_compute_pipeline(aabb_build_pipeline);
    compute_pass.dispatch(dispatch_group_count, 1, 1);

    has_built = true;
    cached_fov_degrees = vertical_fov_degrees;
    cached_aspect_ratio = aspect_ratio;
    cached_near_plane = near_plane;
    cached_far_plane = far_plane;
}

auto SDL_GPUClusterPass::cull_lights(
    CommandBuffer& command_buffer,
    const Light& light_data,
    const Maths::Matrix4x4f& view_matrix
) -> void {
    {
        auto copy_pass = command_buffer.begin_copy_pass();
        upload_lights<AlignedPointLight>(
            copy_pass,
            point_light_transfer_buffer,
            point_light_buffer,
            gsl::span{light_data.point_lights}
        );
        upload_lights<AlignedSpotLight>(
            copy_pass,
            spot_light_transfer_buffer,
            spot_light_buffer,
            gsl::span{light_data.spot_lights}
        );
    }

    const auto cull_params = ClusterCullParams{
        .view_matrix = view_matrix,
        .counts =
            {light_data.point_light_count,
             light_data.spot_light_count,
             cluster_count,
             0},
    };

    const auto point_light_buffers =
        std::array<const Buffer* const, 2>{&point_light_buffer, &spot_light_buffer};

    {
        const auto storage_bindings =
            std::array<StorageBufferReadWriteBinding, 2>{
                StorageBufferReadWriteBinding{
                    .buffer = &cluster_aabb_buffer, .cycle = false
                },
                StorageBufferReadWriteBinding{
                    .buffer = &cluster_light_grid_buffer, .cycle = false
                },
            };
        auto compute_pass = command_buffer.begin_compute_pass({}, storage_bindings);
        compute_pass.bind_storage_buffers(0, point_light_buffers);
        command_buffer.push_compute_uniform_data(
            0,
            gsl::span<const std::byte>{
                reinterpret_cast<const std::byte*>(&cull_params),
                sizeof(cull_params)
            }
        );
        compute_pass.bind_compute_pipeline(light_count_pipeline);
        compute_pass.dispatch(dispatch_group_count, 1, 1);
    }

    {
        const auto scan_params = ClusterScanParams{
            .params = {cluster_count, 0, 0, 0},
        };
        const auto storage_bindings =
            std::array<StorageBufferReadWriteBinding, 1>{
                StorageBufferReadWriteBinding{
                    .buffer = &cluster_light_grid_buffer, .cycle = false
                },
            };
        auto compute_pass = command_buffer.begin_compute_pass({}, storage_bindings);
        command_buffer.push_compute_uniform_data(
            0,
            gsl::span<const std::byte>{
                reinterpret_cast<const std::byte*>(&scan_params),
                sizeof(scan_params)
            }
        );
        compute_pass.bind_compute_pipeline(light_scan_pipeline);
        compute_pass.dispatch(1, 1, 1);
    }

    {
        const auto storage_bindings =
            std::array<StorageBufferReadWriteBinding, 3>{
                StorageBufferReadWriteBinding{
                    .buffer = &cluster_aabb_buffer, .cycle = false
                },
                StorageBufferReadWriteBinding{
                    .buffer = &cluster_light_grid_buffer, .cycle = false
                },
                StorageBufferReadWriteBinding{
                    .buffer = &global_light_index_list_buffer, .cycle = false
                },
            };
        auto compute_pass = command_buffer.begin_compute_pass({}, storage_bindings);
        compute_pass.bind_storage_buffers(0, point_light_buffers);
        command_buffer.push_compute_uniform_data(
            0,
            gsl::span<const std::byte>{
                reinterpret_cast<const std::byte*>(&cull_params),
                sizeof(cull_params)
            }
        );
        compute_pass.bind_compute_pipeline(light_compact_pipeline);
        compute_pass.dispatch(dispatch_group_count, 1, 1);
    }
}

auto SDL_GPUClusterPass::get_cluster_aabb_buffer() const -> const Buffer& {
    return cluster_aabb_buffer;
}

auto SDL_GPUClusterPass::get_cluster_light_grid_buffer() const
    -> const Buffer& {
    return cluster_light_grid_buffer;
}

auto SDL_GPUClusterPass::get_global_light_index_list_buffer() const
    -> const Buffer& {
    return global_light_index_list_buffer;
}

auto SDL_GPUClusterPass::get_point_light_buffer() const -> const Buffer& {
    return point_light_buffer;
}

auto SDL_GPUClusterPass::get_spot_light_buffer() const -> const Buffer& {
    return spot_light_buffer;
}

}  // namespace Luminol::Graphics::SDL_GPU
