#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>

#include <gsl/gsl>
#include <SDL3/SDL_video.h>

#include <LuminolMaths/Matrix.hpp>
#include <LuminolMaths/Transform.hpp>
#include <LuminolMaths/Units/Angle.hpp>
#include <LuminolMaths/Vector.hpp>

#include <LuminolRenderEngine/Graphics/BoundingBox.hpp>
#include <LuminolRenderEngine/Graphics/Frustum.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCommandBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/RenderPasses/SDL_GPUCopyPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/Culling/SDL_GPUCullingUtils.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUDevice.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUFactory.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUInstanceBatch.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUInstanceBufferCache.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/Culling/SDL_GPUInstanceCullPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUMesh.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPURenderer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTexture.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTransferBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTypes.hpp>
#include <LuminolRenderEngine/Window/Window.hpp>

// Validates SDL_GPUInstanceCullPass::cull's per-instance LOD selection by
// independently recomputing, in plain C++, which LOD bucket each of a
// handful of instances at known distances should land in (mirroring
// instance_cull.hlsl's distance-threshold logic), and comparing against the
// GPU-produced per-LOD indirect draw counts + compacted visible instance
// index lists read back to the CPU.
//
// This specifically exercises the real compiled compute shader end-to-end
// (not just CPU logic), so a cross-language struct-layout mismatch between
// SubmeshCullMetadata in SDL_GPUInstanceCullPass.cpp and its mirror in
// instance_cull.hlsl - which silently corrupts which command/offset an
// instance's count and index get written to - shows up as a numeric
// mismatch here instead of only as a visual rendering bug.

namespace {

using namespace Luminol::Graphics;
using namespace Luminol::Graphics::SDL_GPU;
using namespace Luminol::Maths;

constexpr auto vertical_fov_degrees = 45.0F;
constexpr auto camera_aspect_ratio = 1.0F;
constexpr auto near_plane = 0.1F;
// Large enough to keep every on-axis LOD test instance (up to z=150) inside
// the frustum, so only distance (not frustum survival) determines LOD bucket
// for those instances.
constexpr auto far_plane = 500.0F;

// Must match SDL_GPUInstanceCullPass.cpp's default_lod_distances_sq (first
// max_lod_levels - 1 entries - the trailing padding element there has no
// equivalent here).
constexpr auto lod_distance_thresholds_sq =
    std::array<float, max_lod_levels - 1>{
        15.0F * 15.0F, 40.0F * 40.0F, 100.0F * 100.0F
    };

// Mirrors SDL_GPUCullingUtils.cpp's own transform_point (row-vector
// convention, mul(pos, matrix)).
auto transform_point(const Matrix4x4f& matrix, const Vector3f& point) -> Vector3f {
    return Vector3f{
        (point.x() * matrix[0][0]) + (point.y() * matrix[1][0]) +
            (point.z() * matrix[2][0]) + matrix[3][0],
        (point.x() * matrix[0][1]) + (point.y() * matrix[1][1]) +
            (point.z() * matrix[2][1]) + matrix[3][1],
        (point.x() * matrix[0][2]) + (point.y() * matrix[1][2]) +
            (point.z() * matrix[2][2]) + matrix[3][2],
    };
}

auto expected_world_bounds(
    const BoundingBox& local_bounds, const Matrix4x4f& model_matrix
) -> BoundingBox {
    const auto corners = std::array<Vector3f, 8>{
        Vector3f{local_bounds.min.x(), local_bounds.min.y(), local_bounds.min.z()},
        Vector3f{local_bounds.max.x(), local_bounds.min.y(), local_bounds.min.z()},
        Vector3f{local_bounds.min.x(), local_bounds.max.y(), local_bounds.min.z()},
        Vector3f{local_bounds.max.x(), local_bounds.max.y(), local_bounds.min.z()},
        Vector3f{local_bounds.min.x(), local_bounds.min.y(), local_bounds.max.z()},
        Vector3f{local_bounds.max.x(), local_bounds.min.y(), local_bounds.max.z()},
        Vector3f{local_bounds.min.x(), local_bounds.max.y(), local_bounds.max.z()},
        Vector3f{local_bounds.max.x(), local_bounds.max.y(), local_bounds.max.z()},
    };

    auto world_min = transform_point(model_matrix, corners[0]);
    auto world_max = world_min;
    for (const auto& corner : corners) {
        const auto world_corner = transform_point(model_matrix, corner);
        world_min = Vector3f{
            std::min(world_min.x(), world_corner.x()),
            std::min(world_min.y(), world_corner.y()),
            std::min(world_min.z(), world_corner.z()),
        };
        world_max = Vector3f{
            std::max(world_max.x(), world_corner.x()),
            std::max(world_max.y(), world_corner.y()),
            std::max(world_max.z(), world_corner.z()),
        };
    }

    return BoundingBox{.min = world_min, .max = world_max};
}

// Smallest LOD whose switch threshold isn't exceeded, else the coarsest
// level - mirrors instance_cull.hlsl's selection loop exactly.
auto expected_lod_for_distance_sq(float distance_sq) -> std::size_t {
    for (auto lod = std::size_t{0}; lod < lod_distance_thresholds_sq.size();
         ++lod) {
        if (distance_sq < lod_distance_thresholds_sq.at(lod)) {
            return lod;
        }
    }
    return max_lod_levels - 1;
}

struct DownloadedLod {
    IndirectDrawCommand command;
    std::vector<uint32_t> visible_indices;
};

// Downloads every LOD's IndirectDrawCommand and compacted visible-instance
// slice for the given submesh in one copy pass, then submits and waits.
auto download_lods(
    GPUDevice& gpu_device,
    CommandBuffer& command_buffer,
    const SDL_GPUInstanceCullPass& instance_cull_pass,
    const SubmeshCullInfo& submesh_info,
    uint32_t instance_count
) -> std::array<DownloadedLod, max_lod_levels> {
    constexpr auto indirect_command_size = uint32_t{sizeof(IndirectDrawCommand)};
    const auto visible_indices_size = instance_count * static_cast<uint32_t>(sizeof(uint32_t));

    auto indirect_download_buffers = std::vector<TransferBuffer>{};
    auto indices_download_buffers = std::vector<TransferBuffer>{};
    indirect_download_buffers.reserve(max_lod_levels);
    indices_download_buffers.reserve(max_lod_levels);
    for (auto lod = std::size_t{0}; lod < max_lod_levels; ++lod) {
        indirect_download_buffers.push_back(
            gpu_device.create_transfer_buffer(TransferBufferInfo{
                .usage = TransferBufferUsage::Download,
                .size = indirect_command_size,
            })
        );
        indices_download_buffers.push_back(
            gpu_device.create_transfer_buffer(TransferBufferInfo{
                .usage = TransferBufferUsage::Download,
                .size = visible_indices_size,
            })
        );
    }

    {
        auto copy_pass = command_buffer.begin_copy_pass();
        for (auto lod = std::size_t{0}; lod < max_lod_levels; ++lod) {
            copy_pass.download_from_buffer(
                instance_cull_pass.get_indirect_command_buffer(),
                submesh_info.indirect_command_byte_offsets.at(lod),
                indirect_download_buffers.at(lod),
                0,
                indirect_command_size
            );
            copy_pass.download_from_buffer(
                instance_cull_pass.get_visible_instance_indices_buffer(),
                submesh_info.instance_base_offsets.at(lod) *
                    static_cast<uint32_t>(sizeof(uint32_t)),
                indices_download_buffers.at(lod),
                0,
                visible_indices_size
            );
        }
    }
    command_buffer.submit();
    gpu_device.wait_for_idle();

    auto results = std::array<DownloadedLod, max_lod_levels>{};
    for (auto lod = std::size_t{0}; lod < max_lod_levels; ++lod) {
        auto command = IndirectDrawCommand{};
        {
            const auto mapped = indirect_download_buffers.at(lod).map(false);
            std::memcpy(&command, mapped.data(), indirect_command_size);
            indirect_download_buffers.at(lod).unmap();
        }

        auto visible_indices = std::vector<uint32_t>(instance_count);
        {
            const auto mapped = indices_download_buffers.at(lod).map(false);
            std::memcpy(
                visible_indices.data(), mapped.data(), visible_indices_size
            );
            indices_download_buffers.at(lod).unmap();
        }
        visible_indices.resize(command.num_instances);
        std::ranges::sort(visible_indices);

        results.at(lod) = DownloadedLod{
            .command = command, .visible_indices = std::move(visible_indices)
        };
    }

    return results;
}

}  // namespace

auto main() -> int {
    using namespace Luminol;

    auto window = Window{64, 64, "Luminol LOD Selection Smoke Test"};

    auto factory = std::make_shared<SDL_GPUFactory>();
    auto renderer = factory->create_renderer(window);
    auto gpu_device = factory->get_gpu_device();

    const auto renderable_id = factory->create_model("res/models/cube/cube.obj");
    const auto meshes = factory->get_meshes(renderable_id);
    const auto& local_bounds = meshes.front().get_local_bounds();

    // Instances 0-3: on-axis, at increasing distance, each meant to land in
    // a different LOD bucket (see lod_distance_thresholds_sq).
    // Instance 4: far to the side - outside the frustum.
    // Instance 5: behind the camera - outside the frustum.
    const auto model_matrices = std::array<Matrix4x4f, 6>{
        Transform::translate_4x4(Vector3f{0.0F, 0.0F, 5.0F}),    // LOD0
        Transform::translate_4x4(Vector3f{0.0F, 0.0F, 25.0F}),   // LOD1
        Transform::translate_4x4(Vector3f{0.0F, 0.0F, 60.0F}),   // LOD2
        Transform::translate_4x4(Vector3f{0.0F, 0.0F, 150.0F}),  // LOD3
        Transform::translate_4x4(Vector3f{1000.0F, 0.0F, 5.0F}), // off-frustum
        Transform::translate_4x4(Vector3f{0.0F, 0.0F, -5.0F}),   // behind camera
    };

    const auto lod_reference_position = Vector3f{0.0F, 0.0F, 0.0F};

    const auto view_matrix = Transform::left_handed_look_at_matrix(
        Transform::LookAtParams<float>{
            .eye = lod_reference_position,
            .target = Vector3f{0.0F, 0.0F, 1.0F},
            .up_vector = Vector3f{0.0F, 1.0F, 0.0F},
        }
    );
    const auto projection_matrix =
        Transform::left_handed_perspective_projection_matrix(
            Transform::PerspectiveMatrixParams<float>{
                .fov = Units::Degrees_f{vertical_fov_degrees},
                .aspect_ratio = camera_aspect_ratio,
                .near_plane = near_plane,
                .far_plane = far_plane,
            }
        );
    const auto view_projection = view_matrix * projection_matrix;
    const auto frustum_planes = extract_frustum_planes(view_projection);

    auto instance_buffer_cache = SDL_GPUInstanceBufferCache{};
    {
        auto upload_command_buffer = gpu_device->create_command_buffer();
        auto copy_pass = upload_command_buffer.begin_copy_pass();
        instance_buffer_cache.upload(
            *gpu_device, copy_pass, renderable_id, gsl::span{model_matrices}
        );
        upload_command_buffer.submit();
        gpu_device->wait_for_idle();
    }

    const auto instance_batches = std::array<InstanceBatch, 1>{
        InstanceBatch{
            .renderable_id = renderable_id,
            .instance_count = static_cast<uint32_t>(model_matrices.size())
        }
    };

    auto dummy_hiz_texture = gpu_device->create_texture(TextureInfo{
        .width = 1,
        .height = 1,
        .format = TextureFormat::R32_Float,
        .usage = TextureUsage::ComputeStorageRead | TextureUsage::Sampler,
        .mip_levels = 1,
    });
    auto dummy_hiz_sampler = gpu_device->create_sampler(SamplerInfo{
        .filter = SamplerFilter::Nearest,
        .address_mode_u = SamplerAddressMode::ClampToEdge,
        .address_mode_v = SamplerAddressMode::ClampToEdge,
        .max_lod = 0,
    });

    // Independently determine each instance's frustum survival and (if it
    // survives) expected LOD bucket.
    auto expected_per_lod = std::array<std::vector<uint32_t>, max_lod_levels>{};
    for (auto i = uint32_t{0}; i < model_matrices.size(); ++i) {
        const auto world_bounds =
            expected_world_bounds(local_bounds, model_matrices.at(i));
        if (!aabb_in_frustum(frustum_planes, world_bounds.min, world_bounds.max)) {
            continue;
        }

        const auto world_center = Vector3f{
            (world_bounds.min.x() + world_bounds.max.x()) * 0.5F,
            (world_bounds.min.y() + world_bounds.max.y()) * 0.5F,
            (world_bounds.min.z() + world_bounds.max.z()) * 0.5F,
        };
        const auto offset = world_center - lod_reference_position;
        const auto distance_sq =
            (offset.x() * offset.x()) + (offset.y() * offset.y()) +
            (offset.z() * offset.z());

        expected_per_lod.at(expected_lod_for_distance_sq(distance_sq))
            .push_back(i);
    }
    for (auto& bucket : expected_per_lod) {
        std::ranges::sort(bucket);
    }

    auto success = true;

    // Case A: LOD enabled - each surviving instance should land in the LOD
    // bucket its distance to lod_reference_position implies.
    {
        auto instance_cull_pass = SDL_GPUInstanceCullPass{*gpu_device};
        auto command_buffer = gpu_device->create_command_buffer();
        const auto layout = instance_cull_pass.cull(
            *factory,
            command_buffer,
            instance_buffer_cache,
            gsl::span{instance_batches},
            frustum_planes,
            view_projection,
            dummy_hiz_texture,
            dummy_hiz_sampler,
            0,
            lod_reference_position,
            true
        );
        const auto& submesh_info = layout.at(0).at(0);

        const auto downloaded = download_lods(
            *gpu_device, command_buffer, instance_cull_pass, submesh_info,
            static_cast<uint32_t>(model_matrices.size())
        );

        for (auto lod = std::size_t{0}; lod < max_lod_levels; ++lod) {
            const auto& expected = expected_per_lod.at(lod);
            const auto& actual = downloaded.at(lod);
            const auto lod_matches =
                actual.command.num_instances ==
                    static_cast<uint32_t>(expected.size()) &&
                actual.visible_indices == expected;
            if (!lod_matches) {
                success = false;
                std::printf(
                    "LOD selection smoke test FAILED (enable_lod=true) at "
                    "LOD %zu: expected %zu survivors, got num_instances=%u\n",
                    lod, expected.size(), actual.command.num_instances
                );
            }
        }
    }

    // Case B: LOD disabled (mirrors the shadow-cascade cull() call sites) -
    // every surviving instance must land in LOD0 regardless of distance.
    {
        auto instance_cull_pass = SDL_GPUInstanceCullPass{*gpu_device};
        auto command_buffer = gpu_device->create_command_buffer();
        const auto layout = instance_cull_pass.cull(
            *factory,
            command_buffer,
            instance_buffer_cache,
            gsl::span{instance_batches},
            frustum_planes,
            view_projection,
            dummy_hiz_texture,
            dummy_hiz_sampler,
            0,
            Vector3f{},
            false
        );
        const auto& submesh_info = layout.at(0).at(0);

        const auto downloaded = download_lods(
            *gpu_device, command_buffer, instance_cull_pass, submesh_info,
            static_cast<uint32_t>(model_matrices.size())
        );

        auto expected_lod0 = std::vector<uint32_t>{};
        for (const auto& bucket : expected_per_lod) {
            expected_lod0.insert(expected_lod0.end(), bucket.begin(), bucket.end());
        }
        std::ranges::sort(expected_lod0);

        if (downloaded.at(0).command.num_instances !=
                static_cast<uint32_t>(expected_lod0.size()) ||
            downloaded.at(0).visible_indices != expected_lod0) {
            success = false;
            std::printf(
                "LOD selection smoke test FAILED (enable_lod=false): "
                "expected %zu survivors all in LOD0, got num_instances=%u\n",
                expected_lod0.size(), downloaded.at(0).command.num_instances
            );
        }
        for (auto lod = std::size_t{1}; lod < max_lod_levels; ++lod) {
            if (downloaded.at(lod).command.num_instances != 0U) {
                success = false;
                std::printf(
                    "LOD selection smoke test FAILED (enable_lod=false): "
                    "LOD %zu expected 0 instances, got %u\n",
                    lod, downloaded.at(lod).command.num_instances
                );
            }
        }
    }

    if (success) {
        std::printf("LOD selection smoke test PASSED\n");
    }

    return success ? 0 : 1;
}
