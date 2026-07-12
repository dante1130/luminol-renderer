#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <numbers>
#include <vector>

#include <SDL3/SDL_video.h>

#include <LuminolMaths/Matrix.hpp>
#include <LuminolMaths/Transform.hpp>
#include <LuminolMaths/Vector.hpp>

#include <LuminolRenderEngine/Graphics/Light.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUClusterPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCommandBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCopyPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUDevice.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTransferBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTypes.hpp>
#include <LuminolRenderEngine/Window/Window.hpp>

// Validates SDL_GPUClusterPass::cull_lights (count -> scan -> compact) by
// independently recomputing, in plain C++, which lights should match every
// one of the 3456 clusters (mirroring cluster_light_count.hlsl /
// cluster_light_compact.hlsl's math exactly) and comparing against the
// GPU-produced cluster light grid + global light index list read back to
// the CPU.

namespace {

using namespace Luminol::Graphics::SDL_GPU;
using namespace Luminol::Maths;

constexpr auto vertical_fov_degrees = 45.0F;
constexpr auto aspect_ratio = 16.0F / 9.0F;
constexpr auto near_plane = 0.1F;
constexpr auto far_plane = 100.0F;

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

auto light_cull_radius(float r, float g, float b) -> float {
    constexpr auto cutoff = 1.0F / 256.0F;
    const auto intensity = std::max(r, std::max(g, b));
    return std::sqrt(std::max(intensity, 0.0F) / cutoff);
}

auto sphere_intersects_aabb(Vec3 center, float radius, Vec3 box_min, Vec3 box_max) -> bool {
    const auto closest_x = std::clamp(center.x, box_min.x, box_max.x);
    const auto closest_y = std::clamp(center.y, box_min.y, box_max.y);
    const auto closest_z = std::clamp(center.z, box_min.z, box_max.z);
    const auto dx = center.x - closest_x;
    const auto dy = center.y - closest_y;
    const auto dz = center.z - closest_z;
    return (dx * dx + dy * dy + dz * dz) <= radius * radius;
}

// Row-vector * matrix, matching this engine's view/projection convention
// (see LuminolMaths::Transform and pbr_frag.hlsl's use of mul(vec, matrix)).
auto transform_point(const Matrix4x4f& matrix, Vec3 point) -> Vec3 {
    const auto x = point.x * matrix[0][0] + point.y * matrix[1][0] +
        point.z * matrix[2][0] + matrix[3][0];
    const auto y = point.x * matrix[0][1] + point.y * matrix[1][1] +
        point.z * matrix[2][1] + matrix[3][1];
    const auto z = point.x * matrix[0][2] + point.y * matrix[1][2] +
        point.z * matrix[2][2] + matrix[3][2];
    return Vec3{.x = x, .y = y, .z = z};
}

struct TestLight {
    Vec3 position;
    Vec3 color;
};

struct GpuClusterLightGrid {
    uint32_t offset;
    uint32_t point_count;
    uint32_t spot_count;
    uint32_t padding;
};

}  // namespace

auto main() -> int {
    using namespace Luminol;

    const auto test_point_lights = std::array<TestLight, 3>{
        TestLight{.position = {0.0F, 0.0F, 5.0F}, .color = {1.0F, 1.0F, 1.0F}},
        TestLight{.position = {20.0F, 20.0F, 5.0F}, .color = {1.0F, 1.0F, 1.0F}},
        TestLight{.position = {0.0F, 0.0F, 80.0F}, .color = {0.5F, 0.5F, 0.5F}},
    };
    const auto test_spot_lights = std::array<TestLight, 2>{
        TestLight{.position = {0.0F, 0.0F, 5.0F}, .color = {1.0F, 1.0F, 1.0F}},
        TestLight{.position = {-30.0F, 0.0F, 10.0F}, .color = {2.0F, 2.0F, 2.0F}},
    };

    auto window = Window{1, 1, "Luminol Cluster Light Cull Smoke Test"};
    auto gpu_device = std::make_shared<GPUDevice>(
        static_cast<SDL_Window*>(window.get_window_handle())
    );

    auto cluster_pass = SDL_GPUClusterPass{*gpu_device};

    auto light_data = Graphics::Light{};
    light_data.point_light_count = static_cast<uint32_t>(test_point_lights.size());
    for (auto i = size_t{0}; i < test_point_lights.size(); ++i) {
        light_data.point_lights[i] = Graphics::AlignedPointLight{
            .position =
                Vector4f{
                    test_point_lights[i].position.x,
                    test_point_lights[i].position.y,
                    test_point_lights[i].position.z,
                    1.0F,
                },
            .color =
                Vector4f{
                    test_point_lights[i].color.x,
                    test_point_lights[i].color.y,
                    test_point_lights[i].color.z,
                    1.0F,
                },
        };
    }
    light_data.spot_light_count = static_cast<uint32_t>(test_spot_lights.size());
    for (auto i = size_t{0}; i < test_spot_lights.size(); ++i) {
        light_data.spot_lights[i] = Graphics::AlignedSpotLight{
            .position =
                Vector4f{
                    test_spot_lights[i].position.x,
                    test_spot_lights[i].position.y,
                    test_spot_lights[i].position.z,
                    1.0F,
                },
            .direction = Vector4f{0.0F, 0.0F, 1.0F, 0.0F},
            .color =
                Vector3f{
                    test_spot_lights[i].color.x,
                    test_spot_lights[i].color.y,
                    test_spot_lights[i].color.z,
                },
            .cut_off = 0.9F,
            .outer_cut_off = 0.8F,
        };
    }

    const auto view_matrix = Transform::left_handed_look_at_matrix(
        Transform::LookAtParams<float>{
            .eye = {0.0F, 0.0F, 0.0F},
            .target = {0.0F, 0.0F, 1.0F},
            .up_vector = {0.0F, 1.0F, 0.0F},
        }
    );

    auto command_buffer = gpu_device->create_command_buffer();
    cluster_pass.build_cluster_grid(
        command_buffer, vertical_fov_degrees, aspect_ratio, near_plane, far_plane
    );
    cluster_pass.cull_lights(command_buffer, light_data, view_matrix);

    const auto& cluster_light_grid_buffer = cluster_pass.get_cluster_light_grid_buffer();
    const auto& global_light_index_list_buffer =
        cluster_pass.get_global_light_index_list_buffer();

    auto grid_download_buffer = gpu_device->create_transfer_buffer(TransferBufferInfo{
        .usage = TransferBufferUsage::Download,
        .size = cluster_light_grid_buffer.get_size(),
    });
    auto index_download_buffer = gpu_device->create_transfer_buffer(TransferBufferInfo{
        .usage = TransferBufferUsage::Download,
        .size = global_light_index_list_buffer.get_size(),
    });

    {
        auto copy_pass = command_buffer.begin_copy_pass();
        copy_pass.download_from_buffer(
            cluster_light_grid_buffer,
            0,
            grid_download_buffer,
            0,
            cluster_light_grid_buffer.get_size()
        );
        copy_pass.download_from_buffer(
            global_light_index_list_buffer,
            0,
            index_download_buffer,
            0,
            global_light_index_list_buffer.get_size()
        );
    }
    command_buffer.submit();
    gpu_device->wait_for_idle();

    const auto grid_mapped = grid_download_buffer.map(false);
    auto grid_results = std::vector<GpuClusterLightGrid>(cluster_count);
    std::memcpy(grid_results.data(), grid_mapped.data(), grid_mapped.size());
    grid_download_buffer.unmap();

    const auto index_mapped = index_download_buffer.map(false);
    auto index_results =
        std::vector<uint32_t>(global_light_index_capacity);
    std::memcpy(index_results.data(), index_mapped.data(), index_mapped.size());
    index_download_buffer.unmap();

    auto success = true;
    auto mismatch_count = uint32_t{0};

    for (auto cluster_index = uint32_t{0}; cluster_index < cluster_count; ++cluster_index) {
        const auto aabb = expected_aabb(cluster_index);

        auto expected_point_indices = std::vector<uint32_t>{};
        for (auto i = uint32_t{0}; i < test_point_lights.size(); ++i) {
            const auto view_position =
                transform_point(view_matrix, test_point_lights[i].position);
            const auto radius = light_cull_radius(
                test_point_lights[i].color.x,
                test_point_lights[i].color.y,
                test_point_lights[i].color.z
            );
            if (sphere_intersects_aabb(view_position, radius, aabb.min_corner, aabb.max_corner)) {
                expected_point_indices.push_back(i);
            }
        }

        auto expected_spot_indices = std::vector<uint32_t>{};
        for (auto i = uint32_t{0}; i < test_spot_lights.size(); ++i) {
            const auto view_position =
                transform_point(view_matrix, test_spot_lights[i].position);
            const auto radius = light_cull_radius(
                test_spot_lights[i].color.x,
                test_spot_lights[i].color.y,
                test_spot_lights[i].color.z
            );
            if (sphere_intersects_aabb(view_position, radius, aabb.min_corner, aabb.max_corner)) {
                expected_spot_indices.push_back(i);
            }
        }

        const auto& actual_grid = grid_results[cluster_index];
        const auto counts_match =
            actual_grid.point_count == expected_point_indices.size() &&
            actual_grid.spot_count == expected_spot_indices.size();

        auto indices_match = counts_match;
        if (counts_match) {
            for (auto i = size_t{0}; i < expected_point_indices.size(); ++i) {
                if (index_results[actual_grid.offset + i] != expected_point_indices[i]) {
                    indices_match = false;
                }
            }
            for (auto i = size_t{0}; i < expected_spot_indices.size(); ++i) {
                if (index_results
                        [actual_grid.offset + expected_point_indices.size() + i] !=
                    expected_spot_indices[i]) {
                    indices_match = false;
                }
            }
        }

        if (!indices_match) {
            if (mismatch_count < 10) {
                std::printf(
                    "Cluster light cull smoke test FAILED at cluster %u: "
                    "expected point_count=%zu spot_count=%zu, got point_count=%u spot_count=%u\n",
                    cluster_index,
                    expected_point_indices.size(),
                    expected_spot_indices.size(),
                    actual_grid.point_count,
                    actual_grid.spot_count
                );
            }
            mismatch_count++;
            success = false;
        }
    }

    if (success) {
        std::printf(
            "Cluster light cull smoke test PASSED (%u clusters checked, %zu point + %zu spot lights)\n",
            cluster_count,
            test_point_lights.size(),
            test_spot_lights.size()
        );
    } else {
        std::printf(
            "Cluster light cull smoke test FAILED: %u/%u clusters mismatched\n",
            mismatch_count,
            cluster_count
        );
    }

    return success ? 0 : 1;
}
