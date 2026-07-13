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
constexpr auto initial_metadata_capacity = uint32_t{64};
constexpr auto initial_group_capacity = uint32_t{64};

// Mirrors cbuffer InstanceCullParams in instance_cull.hlsl. Per-submesh
// fields (bounds, command_index, instance_base_offset, instance_count) live
// in SubmeshCullMetadata instead, looked up per thread group via
// group_to_submesh - see the doc comment on SDL_GPUInstanceCullPass::cull.
struct InstanceCullParams {
    std::array<Vector4f, 6> frustum_planes;
    Matrix4x4f current_view_projection;
    uint32_t hiz_mip_levels;
    uint32_t group_to_submesh_base;
    std::array<float, 2> hiz_pyramid_size;
};

// One submesh's culling inputs. Mirrors struct SubmeshCullMetadata in
// instance_cull.hlsl exactly (StructuredBuffer element layout, not a cbuffer
// - no 16-byte-vector packing rules, just matching contiguous layout).
struct SubmeshCullMetadata {
    Vector4f local_bounds_min;
    Vector4f local_bounds_max;
    uint32_t command_index;
    uint32_t instance_base_offset;
    uint32_t instance_count;
    uint32_t first_group;
};

// One batch's culling dispatch inputs, built once per frame (cost
// proportional to submesh count, not instance count). group_to_submesh_base
// and total_group_count describe this batch's slice of the group_to_submesh
// buffer, dispatched in one call covering every submesh in the batch.
struct BatchDispatchInfo {
    RenderableId renderable_id;
    uint32_t group_to_submesh_base;
    uint32_t total_group_count;
};

auto make_instance_cull_pipeline(GPUDevice& device) -> ComputePipeline {
    return device.create_compute_pipeline(ComputePipelineInfo{
        .path = "res/shaders/sdl_gpu/instance_cull.hlsl",
        .source_language = ShaderSourceLanguage::Hlsl,
        .sampler_count = 1,
        .readonly_storage_buffer_count = 3,
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

auto make_submesh_metadata_buffer(GPUDevice& device, uint32_t metadata_capacity)
    -> Buffer {
    return device.create_buffer(BufferInfo{
        .usage = BufferUsage::ComputeStorageRead,
        .size = metadata_capacity * static_cast<uint32_t>(sizeof(SubmeshCullMetadata)),
    });
}

auto make_group_to_submesh_buffer(GPUDevice& device, uint32_t group_capacity)
    -> Buffer {
    return device.create_buffer(BufferInfo{
        .usage = BufferUsage::ComputeStorageRead,
        .size = group_capacity * static_cast<uint32_t>(sizeof(uint32_t)),
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
      )},
      submesh_metadata_buffer{
          make_submesh_metadata_buffer(device, initial_metadata_capacity)
      },
      submesh_metadata_transfer_buffer{device.create_transfer_buffer(TransferBufferInfo{
          .usage = TransferBufferUsage::Upload,
          .size =
              initial_metadata_capacity * static_cast<uint32_t>(sizeof(SubmeshCullMetadata)),
      })},
      group_to_submesh_buffer{
          make_group_to_submesh_buffer(device, initial_group_capacity)
      },
      group_to_submesh_transfer_buffer{device.create_transfer_buffer(TransferBufferInfo{
          .usage = TransferBufferUsage::Upload,
          .size = initial_group_capacity * static_cast<uint32_t>(sizeof(uint32_t)),
      })} {}

auto SDL_GPUInstanceCullPass::cull(
    const SDL_GPUFactory& graphics_factory,
    CommandBuffer& command_buffer,
    const SDL_GPUInstanceBufferCache& instance_buffer_cache,
    gsl::span<const InstanceBatch> instance_batches,
    const std::array<Vector4f, 6>& camera_frustum_planes,
    const Matrix4x4f& current_view_projection,
    const Texture& hiz_pyramid,
    const Sampler& hiz_sampler,
    uint32_t hiz_mip_levels
) -> InstanceCullLayout {
    auto layout = InstanceCullLayout{};
    layout.reserve(instance_batches.size());

    auto commands = std::vector<IndirectDrawCommand>{};
    auto submesh_metadata = std::vector<SubmeshCullMetadata>{};
    auto group_to_submesh = std::vector<uint32_t>{};
    auto batch_dispatch_infos = std::vector<BatchDispatchInfo>{};

    auto running_index_base = uint32_t{0};

    for (const auto& batch : instance_batches) {
        const auto meshes = graphics_factory.get_meshes(batch.renderable_id);
        auto submesh_infos = std::vector<SubmeshCullInfo>{};
        submesh_infos.reserve(meshes.size());

        const auto batch_group_base =
            static_cast<uint32_t>(group_to_submesh.size());
        auto batch_group_count = uint32_t{0};

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

            const auto local_bounds = mesh.get_local_bounds();
            const auto submesh_index =
                static_cast<uint32_t>(submesh_metadata.size());
            const auto first_group = batch_group_base + batch_group_count;
            const auto group_count = (batch.instance_count + threads_per_group - 1) /
                threads_per_group;

            submesh_metadata.push_back(SubmeshCullMetadata{
                .local_bounds_min = Vector4f{
                    local_bounds.min.x(), local_bounds.min.y(),
                    local_bounds.min.z(), 0.0F
                },
                .local_bounds_max = Vector4f{
                    local_bounds.max.x(), local_bounds.max.y(),
                    local_bounds.max.z(), 0.0F
                },
                .command_index = command_index,
                .instance_base_offset = instance_base_offset,
                .instance_count = batch.instance_count,
                .first_group = first_group,
            });

            group_to_submesh.insert(
                group_to_submesh.end(), group_count, submesh_index
            );
            batch_group_count += group_count;
        }

        layout.push_back(std::move(submesh_infos));

        if (batch_group_count > 0) {
            batch_dispatch_infos.push_back(BatchDispatchInfo{
                .renderable_id = batch.renderable_id,
                .group_to_submesh_base = batch_group_base,
                .total_group_count = batch_group_count,
            });
        }
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

    const auto required_metadata_size = static_cast<uint32_t>(
        submesh_metadata.size() * sizeof(SubmeshCullMetadata)
    );
    if (submesh_metadata_buffer.get_size() < required_metadata_size) {
        submesh_metadata_buffer = make_submesh_metadata_buffer(
            *graphics_factory.get_gpu_device(),
            static_cast<uint32_t>(submesh_metadata.size())
        );
    }
    if (submesh_metadata_transfer_buffer.get_size() < required_metadata_size) {
        submesh_metadata_transfer_buffer =
            graphics_factory.get_gpu_device()->create_transfer_buffer(TransferBufferInfo{
                .usage = TransferBufferUsage::Upload,
                .size = required_metadata_size,
            });
    }

    const auto required_group_size = static_cast<uint32_t>(
        group_to_submesh.size() * sizeof(uint32_t)
    );
    if (group_to_submesh_buffer.get_size() < required_group_size) {
        group_to_submesh_buffer = make_group_to_submesh_buffer(
            *graphics_factory.get_gpu_device(),
            static_cast<uint32_t>(group_to_submesh.size())
        );
    }
    if (group_to_submesh_transfer_buffer.get_size() < required_group_size) {
        group_to_submesh_transfer_buffer =
            graphics_factory.get_gpu_device()->create_transfer_buffer(TransferBufferInfo{
                .usage = TransferBufferUsage::Upload,
                .size = required_group_size,
            });
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

    if (!submesh_metadata.empty()) {
        auto copy_pass = command_buffer.begin_copy_pass();
        const auto mapped = submesh_metadata_transfer_buffer.map(true);
        std::memcpy(
            mapped.data(), submesh_metadata.data(), required_metadata_size
        );
        submesh_metadata_transfer_buffer.unmap();
        copy_pass.upload_to_buffer(
            submesh_metadata_transfer_buffer, 0, submesh_metadata_buffer, 0,
            required_metadata_size, true
        );
    }

    if (!group_to_submesh.empty()) {
        auto copy_pass = command_buffer.begin_copy_pass();
        const auto mapped = group_to_submesh_transfer_buffer.map(true);
        std::memcpy(
            mapped.data(), group_to_submesh.data(), required_group_size
        );
        group_to_submesh_transfer_buffer.unmap();
        copy_pass.upload_to_buffer(
            group_to_submesh_transfer_buffer, 0, group_to_submesh_buffer, 0,
            required_group_size, true
        );
    }

    if (!batch_dispatch_infos.empty()) {
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

        for (const auto& info : batch_dispatch_infos) {
            const auto& instance_models_buffer =
                instance_buffer_cache.get(info.renderable_id);
            const auto read_only_bindings = std::array<const Buffer* const, 3>{
                &instance_models_buffer, &submesh_metadata_buffer,
                &group_to_submesh_buffer
            };
            compute_pass.bind_storage_buffers(0, read_only_bindings);

            const auto params = InstanceCullParams{
                .frustum_planes = camera_frustum_planes,
                .current_view_projection = current_view_projection,
                .hiz_mip_levels = hiz_mip_levels,
                .group_to_submesh_base = info.group_to_submesh_base,
                .hiz_pyramid_size = hiz_pyramid_size,
            };
            command_buffer.push_compute_uniform_data(
                0,
                gsl::span<const std::byte>{
                    reinterpret_cast<const std::byte*>(&params), sizeof(params)
                }
            );

            compute_pass.dispatch(info.total_group_count, 1, 1);
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
