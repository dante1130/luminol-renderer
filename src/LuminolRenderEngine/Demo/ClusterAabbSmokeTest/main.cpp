#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <numbers>
#include <vector>

#include <SDL3/SDL_video.h>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUClusterPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCommandBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCopyPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUDevice.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTransferBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTypes.hpp>
#include <LuminolRenderEngine/Window/Window.hpp>

// Validates SDL_GPUClusterPass::build_cluster_grid by independently
// recomputing the expected view-space AABB (in plain C++, mirroring
// cluster_aabb_build.hlsl's math) for a handful of clusters and comparing
// against the GPU-produced buffer read back to the CPU.

namespace {

using namespace Luminol::Graphics::SDL_GPU;

constexpr auto vertical_fov_degrees = 45.0F;
constexpr auto aspect_ratio = 16.0F / 9.0F;
constexpr auto near_plane = 0.1F;
constexpr auto far_plane = 100.0F;
constexpr auto epsilon = 0.01F;

struct Vec3 {
    float x;
    float y;
    float z;
};

struct AabbCpu {
    Vec3 min_corner;
    Vec3 max_corner;
};

auto corner_view_position(
    float x_ndc, float y_ndc, float z_view, float tan_half_fov_y, float aspect
) -> Vec3 {
    return Vec3{
        .x = x_ndc * z_view * aspect * tan_half_fov_y,
        .y = y_ndc * z_view * tan_half_fov_y,
        .z = z_view,
    };
}

auto expected_aabb(uint32_t cluster_index) -> AabbCpu {
    const auto tiles_per_slice = cluster_grid_x * cluster_grid_y;
    const auto cz = cluster_index / tiles_per_slice;
    const auto slice_remainder = cluster_index % tiles_per_slice;
    const auto cy = slice_remainder / cluster_grid_x;
    const auto cx = slice_remainder % cluster_grid_x;

    const auto tan_half_fov_y = std::tan(
        vertical_fov_degrees * (std::numbers::pi_v<float> / 180.0F) / 2.0F
    );

    const auto slice_ratio = far_plane / near_plane;
    const auto z_near_slice = near_plane *
        std::pow(slice_ratio, static_cast<float>(cz) / static_cast<float>(cluster_grid_z));
    const auto z_far_slice = near_plane *
        std::pow(slice_ratio, static_cast<float>(cz + 1) / static_cast<float>(cluster_grid_z));

    const auto x_ndc_min = -1.0F + 2.0F * static_cast<float>(cx) / static_cast<float>(cluster_grid_x);
    const auto x_ndc_max = -1.0F + 2.0F * static_cast<float>(cx + 1) / static_cast<float>(cluster_grid_x);
    const auto y_ndc_min = -1.0F + 2.0F * static_cast<float>(cy) / static_cast<float>(cluster_grid_y);
    const auto y_ndc_max = -1.0F + 2.0F * static_cast<float>(cy + 1) / static_cast<float>(cluster_grid_y);

    const auto corners = std::array<Vec3, 8>{
        corner_view_position(x_ndc_min, y_ndc_min, z_near_slice, tan_half_fov_y, aspect_ratio),
        corner_view_position(x_ndc_max, y_ndc_min, z_near_slice, tan_half_fov_y, aspect_ratio),
        corner_view_position(x_ndc_min, y_ndc_max, z_near_slice, tan_half_fov_y, aspect_ratio),
        corner_view_position(x_ndc_max, y_ndc_max, z_near_slice, tan_half_fov_y, aspect_ratio),
        corner_view_position(x_ndc_min, y_ndc_min, z_far_slice, tan_half_fov_y, aspect_ratio),
        corner_view_position(x_ndc_max, y_ndc_min, z_far_slice, tan_half_fov_y, aspect_ratio),
        corner_view_position(x_ndc_min, y_ndc_max, z_far_slice, tan_half_fov_y, aspect_ratio),
        corner_view_position(x_ndc_max, y_ndc_max, z_far_slice, tan_half_fov_y, aspect_ratio),
    };

    auto result = AabbCpu{.min_corner = corners[0], .max_corner = corners[0]};
    for (const auto& corner : corners) {
        result.min_corner.x = std::min(result.min_corner.x, corner.x);
        result.min_corner.y = std::min(result.min_corner.y, corner.y);
        result.min_corner.z = std::min(result.min_corner.z, corner.z);
        result.max_corner.x = std::max(result.max_corner.x, corner.x);
        result.max_corner.y = std::max(result.max_corner.y, corner.y);
        result.max_corner.z = std::max(result.max_corner.z, corner.z);
    }
    return result;
}

struct GpuClusterAabb {
    float min_view[4];
    float max_view[4];
};

auto approx_equal(float lhs, float rhs) -> bool {
    return std::fabs(lhs - rhs) <= epsilon;
}

}  // namespace

auto main() -> int {
    using namespace Luminol;

    auto window = Window{
        1, 1, "Luminol Cluster AABB Smoke Test", Graphics::GraphicsApi::SDL_GPU
    };
    auto gpu_device = std::make_shared<GPUDevice>(
        static_cast<SDL_Window*>(window.get_window_handle())
    );

    auto cluster_pass = SDL_GPUClusterPass{*gpu_device};

    auto command_buffer = gpu_device->create_command_buffer();
    cluster_pass.build_cluster_grid(
        command_buffer, vertical_fov_degrees, aspect_ratio, near_plane, far_plane
    );

    const auto& cluster_aabb_buffer = cluster_pass.get_cluster_aabb_buffer();
    auto download_transfer_buffer =
        gpu_device->create_transfer_buffer(TransferBufferInfo{
            .usage = TransferBufferUsage::Download,
            .size = cluster_aabb_buffer.get_size(),
        });

    {
        auto copy_pass = command_buffer.begin_copy_pass();
        copy_pass.download_from_buffer(
            cluster_aabb_buffer,
            0,
            download_transfer_buffer,
            0,
            cluster_aabb_buffer.get_size()
        );
    }
    command_buffer.submit();
    gpu_device->wait_for_idle();

    const auto mapped = download_transfer_buffer.map(false);
    auto results = std::vector<GpuClusterAabb>(cluster_count);
    std::memcpy(results.data(), mapped.data(), mapped.size());
    download_transfer_buffer.unmap();

    const auto indices_to_check = std::array<uint32_t, 3>{
        0, cluster_count / 2, cluster_count - 1
    };

    auto success = true;
    for (const auto cluster_index : indices_to_check) {
        const auto expected = expected_aabb(cluster_index);
        const auto& actual = results[cluster_index];

        const auto matches = approx_equal(expected.min_corner.x, actual.min_view[0]) &&
            approx_equal(expected.min_corner.y, actual.min_view[1]) &&
            approx_equal(expected.min_corner.z, actual.min_view[2]) &&
            approx_equal(expected.max_corner.x, actual.max_view[0]) &&
            approx_equal(expected.max_corner.y, actual.max_view[1]) &&
            approx_equal(expected.max_corner.z, actual.max_view[2]);

        if (!matches) {
            std::printf(
                "Cluster AABB smoke test FAILED at cluster %u: "
                "expected min(%f,%f,%f) max(%f,%f,%f), got min(%f,%f,%f) max(%f,%f,%f)\n",
                cluster_index,
                expected.min_corner.x,
                expected.min_corner.y,
                expected.min_corner.z,
                expected.max_corner.x,
                expected.max_corner.y,
                expected.max_corner.z,
                actual.min_view[0],
                actual.min_view[1],
                actual.min_view[2],
                actual.max_view[0],
                actual.max_view[1],
                actual.max_view[2]
            );
            success = false;
        }
    }

    if (success) {
        std::printf(
            "Cluster AABB smoke test PASSED (%zu clusters checked, %u total)\n",
            indices_to_check.size(),
            cluster_count
        );
    }

    return success ? 0 : 1;
}
