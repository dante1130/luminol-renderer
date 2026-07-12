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
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCopyPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCullingUtils.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUDevice.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUFactory.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUInstanceBatch.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUInstanceBufferCache.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUInstanceCullPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUMesh.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPURenderer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTexture.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTransferBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTypes.hpp>
#include <LuminolRenderEngine/Window/Window.hpp>

// Validates SDL_GPUInstanceCullPass::cull (frustum-only, hiz_mip_levels=0) by
// independently recomputing, in plain C++, which of a handful of instances
// should survive an AABB-vs-frustum test (mirroring instance_cull.hlsl's
// math via the already-tested Luminol::Graphics::aabb_in_frustum), and
// comparing against the GPU-produced indirect draw count + compacted visible
// instance index list read back to the CPU.
//
// Unlike ClusterLightCullSmokeTest's per-cluster serial light loop (which
// preserves ascending light-index order deterministically),
// visible_instance_indices is populated via one thread per instance racing
// an atomic counter, so surviving indices are compared as a sorted set, not
// positionally.

namespace {

using namespace Luminol::Graphics;
using namespace Luminol::Graphics::SDL_GPU;
using namespace Luminol::Maths;

constexpr auto vertical_fov_degrees = 45.0F;
constexpr auto camera_aspect_ratio = 1.0F;
constexpr auto near_plane = 0.1F;
constexpr auto far_plane = 100.0F;

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

}  // namespace

auto main() -> int {
    using namespace Luminol;

    auto window = Window{64, 64, "Luminol Instance Cull Smoke Test"};

    auto factory = std::make_shared<SDL_GPUFactory>();
    auto renderer = factory->create_renderer(window);
    auto gpu_device = factory->get_gpu_device();

    const auto renderable_id = factory->create_model("res/models/cube/cube.obj");
    const auto meshes = factory->get_meshes(renderable_id);
    const auto& local_bounds = meshes.front().get_local_bounds();

    // Instance 0: near-center, inside the frustum.
    // Instance 1: far but within far_plane, inside the frustum.
    // Instance 2: far to the side, outside the frustum.
    // Instance 3: behind the camera, outside the frustum.
    const auto model_matrices = std::array<Matrix4x4f, 4>{
        Transform::translate_4x4(Vector3f{0.0F, 0.0F, 5.0F}),
        Transform::translate_4x4(Vector3f{0.0F, 0.0F, 50.0F}),
        Transform::translate_4x4(Vector3f{1000.0F, 0.0F, 5.0F}),
        Transform::translate_4x4(Vector3f{0.0F, 0.0F, -5.0F}),
    };

    const auto view_matrix = Transform::left_handed_look_at_matrix(
        Transform::LookAtParams<float>{
            .eye = Vector3f{0.0F, 0.0F, 0.0F},
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

    auto command_buffer = gpu_device->create_command_buffer();
    {
        auto copy_pass = command_buffer.begin_copy_pass();
        instance_buffer_cache.upload(
            *gpu_device, copy_pass, renderable_id, gsl::span{model_matrices}
        );
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

    auto instance_cull_pass = SDL_GPUInstanceCullPass{*gpu_device};
    const auto layout = instance_cull_pass.cull(
        *factory,
        command_buffer,
        instance_buffer_cache,
        gsl::span{instance_batches},
        frustum_planes,
        view_projection,
        dummy_hiz_texture,
        dummy_hiz_sampler,
        0
    );

    const auto& submesh_info = layout.at(0).at(0);

    constexpr auto indirect_command_size = uint32_t{sizeof(IndirectDrawCommand)};
    const auto max_visible_indices_size =
        static_cast<uint32_t>(model_matrices.size() * sizeof(uint32_t));

    auto indirect_download_buffer =
        gpu_device->create_transfer_buffer(TransferBufferInfo{
            .usage = TransferBufferUsage::Download,
            .size = indirect_command_size,
        });
    auto indices_download_buffer =
        gpu_device->create_transfer_buffer(TransferBufferInfo{
            .usage = TransferBufferUsage::Download,
            .size = max_visible_indices_size,
        });

    {
        auto copy_pass = command_buffer.begin_copy_pass();
        copy_pass.download_from_buffer(
            instance_cull_pass.get_indirect_command_buffer(),
            submesh_info.indirect_command_byte_offset,
            indirect_download_buffer,
            0,
            indirect_command_size
        );
        copy_pass.download_from_buffer(
            instance_cull_pass.get_visible_instance_indices_buffer(),
            submesh_info.instance_base_offset * static_cast<uint32_t>(sizeof(uint32_t)),
            indices_download_buffer,
            0,
            max_visible_indices_size
        );
    }
    command_buffer.submit();
    gpu_device->wait_for_idle();

    auto indirect_command = IndirectDrawCommand{};
    {
        const auto mapped = indirect_download_buffer.map(false);
        std::memcpy(&indirect_command, mapped.data(), indirect_command_size);
        indirect_download_buffer.unmap();
    }

    auto visible_indices = std::vector<uint32_t>(model_matrices.size());
    {
        const auto mapped = indices_download_buffer.map(false);
        std::memcpy(visible_indices.data(), mapped.data(), max_visible_indices_size);
        indices_download_buffer.unmap();
    }
    visible_indices.resize(indirect_command.num_instances);

    auto expected_survivors = std::vector<uint32_t>{};
    for (auto i = uint32_t{0}; i < model_matrices.size(); ++i) {
        const auto world_bounds = expected_world_bounds(local_bounds, model_matrices[i]);
        if (aabb_in_frustum(frustum_planes, world_bounds.min, world_bounds.max)) {
            expected_survivors.push_back(i);
        }
    }

    std::ranges::sort(visible_indices);
    std::ranges::sort(expected_survivors);

    auto success = indirect_command.num_instances ==
        static_cast<uint32_t>(expected_survivors.size()) &&
        visible_indices == expected_survivors;

    if (!success) {
        std::printf(
            "Instance cull smoke test FAILED: expected %zu survivors, got "
            "num_instances=%u\n",
            expected_survivors.size(),
            indirect_command.num_instances
        );
    } else {
        std::printf(
            "Instance cull smoke test PASSED (%zu of %zu instances survived)\n",
            expected_survivors.size(),
            model_matrices.size()
        );
    }

    return success ? 0 : 1;
}
