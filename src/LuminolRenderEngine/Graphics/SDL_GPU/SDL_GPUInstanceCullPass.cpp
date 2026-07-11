#include "SDL_GPUInstanceCullPass.hpp"

#include <cstring>

#include <gsl/gsl>

#include <LuminolRenderEngine/Graphics/BoundingBox.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCommandBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUComputePass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCopyPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCullingUtils.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUDevice.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUFactory.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUInstanceBufferCache.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUMesh.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTexture.hpp>

namespace Luminol::Graphics::SDL_GPU {

namespace {

using namespace Luminol::Maths;

constexpr auto threads_per_group = uint32_t{64};

// Modest initial capacities, grown in place (like SDL_GPUInstanceBufferCache)
// as larger frames are seen - never recreated once large enough.
constexpr auto initial_command_capacity = uint32_t{64};
constexpr auto initial_visible_index_capacity = uint32_t{4096};

// Mirrors cbuffer InstanceCullParams in instance_cull.hlsl.
struct InstanceCullParams {
    std::array<Vector4f, 6> frustum_planes;
    Vector4f local_bounds_min;
    Vector4f local_bounds_max;
    Matrix4x4f previous_view_projection;
    uint32_t command_index;
    uint32_t instance_base_offset;
    uint32_t instance_count;
    uint32_t hiz_mip_levels;
    std::array<float, 2> hiz_pyramid_size;
    std::array<float, 2> padding;
};

// One submesh's culling dispatch inputs, built once per frame (cost
// proportional to submesh count, not instance count).
struct DispatchInfo {
    RenderableId renderable_id;
    BoundingBox local_bounds;
    uint32_t instance_count;
    uint32_t command_index;
    uint32_t instance_base_offset;
};

auto make_instance_cull_pipeline(GPUDevice& device) -> ComputePipeline {
    return device.create_compute_pipeline(ComputePipelineInfo{
        .path = "res/shaders/sdl_gpu/instance_cull.hlsl",
        .source_language = ShaderSourceLanguage::Hlsl,
        .sampler_count = 1,
        .readonly_storage_buffer_count = 1,
        .readwrite_storage_buffer_count = 2,
        .uniform_buffer_count = 1,
        .threadcount_x = threads_per_group,
        .threadcount_y = 1,
        .threadcount_z = 1,
    });
}

auto make_indirect_command_buffer(GPUDevice& device, uint32_t command_capacity)
    -> Buffer {
    return device.create_buffer(BufferInfo{
        .usage = BufferUsage::ComputeStorageReadWrite | BufferUsage::Indirect,
        .size = command_capacity * static_cast<uint32_t>(sizeof(IndirectDrawCommand)),
    });
}

auto make_visible_instance_indices_buffer(GPUDevice& device, uint32_t index_capacity)
    -> Buffer {
    return device.create_buffer(BufferInfo{
        .usage = BufferUsage::ComputeStorageReadWrite | BufferUsage::StorageRead,
        .size = index_capacity * static_cast<uint32_t>(sizeof(uint32_t)),
    });
}

}  // namespace

SDL_GPUInstanceCullPass::SDL_GPUInstanceCullPass(GPUDevice& device)
    : instance_cull_pipeline{make_instance_cull_pipeline(device)},
      indirect_command_buffer{
          make_indirect_command_buffer(device, initial_command_capacity)
      },
      indirect_command_transfer_buffer{device.create_transfer_buffer(TransferBufferInfo{
          .usage = TransferBufferUsage::Upload,
          .size =
              initial_command_capacity * static_cast<uint32_t>(sizeof(IndirectDrawCommand)),
      })},
      visible_instance_indices_buffer{make_visible_instance_indices_buffer(
          device, initial_visible_index_capacity
      )} {}

auto SDL_GPUInstanceCullPass::cull(
    const SDL_GPUFactory& graphics_factory,
    CommandBuffer& command_buffer,
    const SDL_GPUInstanceBufferCache& instance_buffer_cache,
    gsl::span<const InstanceBatch> instance_batches,
    const std::array<Vector4f, 6>& camera_frustum_planes,
    const Matrix4x4f& previous_view_projection,
    const Texture& hiz_pyramid,
    const Sampler& hiz_sampler,
    uint32_t hiz_mip_levels
) -> InstanceCullLayout {
    auto layout = InstanceCullLayout{};
    layout.reserve(instance_batches.size());

    auto commands = std::vector<IndirectDrawCommand>{};
    auto dispatch_infos = std::vector<DispatchInfo>{};

    auto running_index_base = uint32_t{0};

    for (const auto& batch : instance_batches) {
        const auto meshes = graphics_factory.get_meshes(batch.renderable_id);
        auto submesh_infos = std::vector<SubmeshCullInfo>{};
        submesh_infos.reserve(meshes.size());

        for (const auto& mesh : meshes) {
            const auto command_index = static_cast<uint32_t>(commands.size());
            commands.push_back(IndirectDrawCommand{
                .num_indices = mesh.get_index_count(),
                .num_instances = 0U,
                .first_index = mesh.get_first_index(),
                .vertex_offset = mesh.get_vertex_offset(),
                .first_instance = 0U,
            });

            const auto instance_base_offset = running_index_base;
            running_index_base += batch.instance_count;

            submesh_infos.push_back(SubmeshCullInfo{
                .indirect_command_byte_offset =
                    command_index * static_cast<uint32_t>(sizeof(IndirectDrawCommand)),
                .instance_base_offset = instance_base_offset,
            });

            dispatch_infos.push_back(DispatchInfo{
                .renderable_id = batch.renderable_id,
                .local_bounds = mesh.get_local_bounds(),
                .instance_count = batch.instance_count,
                .command_index = command_index,
                .instance_base_offset = instance_base_offset,
            });
        }

        layout.push_back(std::move(submesh_infos));
    }

    const auto required_command_size = static_cast<uint32_t>(
        commands.size() * sizeof(IndirectDrawCommand)
    );
    if (indirect_command_buffer.get_size() < required_command_size) {
        indirect_command_buffer = make_indirect_command_buffer(
            *graphics_factory.get_gpu_device(),
            static_cast<uint32_t>(commands.size())
        );
    }
    if (indirect_command_transfer_buffer.get_size() < required_command_size) {
        indirect_command_transfer_buffer =
            graphics_factory.get_gpu_device()->create_transfer_buffer(TransferBufferInfo{
                .usage = TransferBufferUsage::Upload,
                .size = required_command_size,
            });
    }

    const auto required_index_size =
        running_index_base * static_cast<uint32_t>(sizeof(uint32_t));
    if (visible_instance_indices_buffer.get_size() < required_index_size) {
        visible_instance_indices_buffer = make_visible_instance_indices_buffer(
            *graphics_factory.get_gpu_device(), running_index_base
        );
    }

    if (!commands.empty()) {
        auto copy_pass = command_buffer.begin_copy_pass();
        const auto mapped = indirect_command_transfer_buffer.map(true);
        std::memcpy(mapped.data(), commands.data(), required_command_size);
        indirect_command_transfer_buffer.unmap();
        copy_pass.upload_to_buffer(
            indirect_command_transfer_buffer, 0, indirect_command_buffer, 0,
            required_command_size, true
        );
    }

    if (!dispatch_infos.empty()) {
        const auto storage_bindings = std::array<StorageBufferReadWriteBinding, 2>{
            StorageBufferReadWriteBinding{
                .buffer = &indirect_command_buffer, .cycle = false
            },
            StorageBufferReadWriteBinding{
                .buffer = &visible_instance_indices_buffer, .cycle = false
            },
        };
        auto compute_pass = command_buffer.begin_compute_pass({}, storage_bindings);
        compute_pass.bind_compute_pipeline(instance_cull_pipeline);

        const auto hiz_sampler_bindings = std::array{TextureSamplerBinding{
            .texture = &hiz_pyramid, .sampler = &hiz_sampler
        }};
        compute_pass.bind_samplers(0, hiz_sampler_bindings);

        const auto hiz_pyramid_size = std::array<float, 2>{
            static_cast<float>(hiz_pyramid.get_width()),
            static_cast<float>(hiz_pyramid.get_height()),
        };

        for (const auto& info : dispatch_infos) {
            const auto& instance_models_buffer =
                instance_buffer_cache.get(info.renderable_id);
            const auto read_only_bindings =
                std::array<const Buffer* const, 1>{&instance_models_buffer};
            compute_pass.bind_storage_buffers(0, read_only_bindings);

            const auto params = InstanceCullParams{
                .frustum_planes = camera_frustum_planes,
                .local_bounds_min = Vector4f{
                    info.local_bounds.min.x(), info.local_bounds.min.y(),
                    info.local_bounds.min.z(), 0.0F
                },
                .local_bounds_max = Vector4f{
                    info.local_bounds.max.x(), info.local_bounds.max.y(),
                    info.local_bounds.max.z(), 0.0F
                },
                .previous_view_projection = previous_view_projection,
                .command_index = info.command_index,
                .instance_base_offset = info.instance_base_offset,
                .instance_count = info.instance_count,
                .hiz_mip_levels = hiz_mip_levels,
                .hiz_pyramid_size = hiz_pyramid_size,
                .padding = {0.0F, 0.0F},
            };
            command_buffer.push_compute_uniform_data(
                0,
                gsl::span<const std::byte>{
                    reinterpret_cast<const std::byte*>(&params), sizeof(params)
                }
            );

            const auto group_count =
                (info.instance_count + threads_per_group - 1) / threads_per_group;
            compute_pass.dispatch(group_count, 1, 1);
        }
    }

    return layout;
}

auto SDL_GPUInstanceCullPass::get_indirect_command_buffer() const
    -> const Buffer& {
    return indirect_command_buffer;
}

auto SDL_GPUInstanceCullPass::get_visible_instance_indices_buffer() const
    -> const Buffer& {
    return visible_instance_indices_buffer;
}

}  // namespace Luminol::Graphics::SDL_GPU
