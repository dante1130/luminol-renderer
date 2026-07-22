#include "SDL_GPUMeshletCullPass.hpp"

#include <cstring>

#include <gsl/gsl>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCommandBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/RenderPasses/SDL_GPUComputePass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/RenderPasses/SDL_GPUCopyPass.hpp>
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

// Must match SDL_GPUMesh.cpp's meshlet_max_triangles and meshlet_cull.hlsl's
// MESHLET_MAX_TRIANGLES.
constexpr auto meshlet_max_triangles = uint32_t{64};
constexpr auto meshlet_fixed_vertex_count = meshlet_max_triangles * 3U;

constexpr auto initial_command_capacity = uint32_t{64};
constexpr auto initial_visible_instance_capacity = uint32_t{4096};
constexpr auto initial_metadata_capacity = uint32_t{64};
constexpr auto initial_group_capacity = uint32_t{64};

// Non-indexed indirect draw command - mirrors SDL_GPUIndirectDrawCommand
// (SDL_gpu.h) and meshlet_cull.hlsl's IndirectDrawCommand exactly. 16
// bytes, already a multiple of the 16-byte StructuredBuffer element stride.
struct MeshletIndirectDrawCommand {
    uint32_t num_vertices;
    uint32_t num_instances;
    uint32_t first_vertex;
    uint32_t first_instance;
};

// Mirrors cbuffer MeshletCullParams in meshlet_cull.hlsl.
struct MeshletCullParams {
    std::array<Vector4f, 6> frustum_planes;
    Matrix4x4f current_view_projection;
    uint32_t hiz_mip_levels;
    uint32_t group_to_meshlet_dispatch_base;
    std::array<float, 2> hiz_pyramid_size;
};

// One (submesh, LOD)'s Phase B dispatch inputs. Mirrors struct
// MeshletCullMetadata in meshlet_cull.hlsl exactly (StructuredBuffer element
// layout). 8 uint32 fields, 32 bytes - already a multiple of the 16-byte
// element stride, no padding needed (see SDL_GPUInstanceCullPass.cpp's
// SubmeshCullMetadata for why that multiple matters in general).
struct MeshletCullMetadata {
    uint32_t phase_a_command_index;
    uint32_t phase_a_instance_base;
    uint32_t meshlet_first;
    uint32_t meshlet_count;
    uint32_t worst_case_instance_count;
    uint32_t output_command_index;
    uint32_t output_instance_base;
    uint32_t first_group;
};

// One batch's Phase B dispatch inputs, built once per cull() call - mirrors
// SDL_GPUInstanceCullPass.cpp's BatchDispatchInfo.
struct BatchDispatchInfo {
    RenderableId renderable_id;
    uint32_t group_to_meshlet_dispatch_base;
    uint32_t total_group_count;
};

auto make_meshlet_cull_pipeline(GPUDevice& device) -> ComputePipeline {
    return device.create_compute_pipeline(ComputePipelineInfo{
        .path = "res/shaders/sdl_gpu/meshlet_cull.hlsl",
        .source_language = ShaderSourceLanguage::Hlsl,
        .sampler_count = 1,
        .readonly_storage_buffer_count = 6,
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
        .size = command_capacity *
            static_cast<uint32_t>(sizeof(MeshletIndirectDrawCommand)),
    });
}

auto make_visible_meshlet_instances_buffer(GPUDevice& device, uint32_t capacity)
    -> Buffer {
    return device.create_buffer(BufferInfo{
        .usage = BufferUsage::ComputeStorageReadWrite | BufferUsage::StorageRead,
        .size = capacity * static_cast<uint32_t>(sizeof(std::array<uint32_t, 2>)),
    });
}

auto make_meshlet_cull_metadata_buffer(GPUDevice& device, uint32_t capacity)
    -> Buffer {
    return device.create_buffer(BufferInfo{
        .usage = BufferUsage::ComputeStorageRead,
        .size = capacity * static_cast<uint32_t>(sizeof(MeshletCullMetadata)),
    });
}

auto make_group_to_meshlet_dispatch_buffer(GPUDevice& device, uint32_t capacity)
    -> Buffer {
    return device.create_buffer(BufferInfo{
        .usage = BufferUsage::ComputeStorageRead,
        .size = capacity * static_cast<uint32_t>(sizeof(uint32_t)),
    });
}

}  // namespace

SDL_GPUMeshletCullPass::SDL_GPUMeshletCullPass(GPUDevice& device)
    : meshlet_cull_pipeline{make_meshlet_cull_pipeline(device)},
      indirect_command_buffer{
          make_indirect_command_buffer(device, initial_command_capacity)
      },
      indirect_command_transfer_buffer{device.create_transfer_buffer(TransferBufferInfo{
          .usage = TransferBufferUsage::Upload,
          .size = initial_command_capacity *
              static_cast<uint32_t>(sizeof(MeshletIndirectDrawCommand)),
      })},
      visible_meshlet_instances_buffer{make_visible_meshlet_instances_buffer(
          device, initial_visible_instance_capacity
      )},
      meshlet_cull_metadata_buffer{
          make_meshlet_cull_metadata_buffer(device, initial_metadata_capacity)
      },
      meshlet_cull_metadata_transfer_buffer{device.create_transfer_buffer(TransferBufferInfo{
          .usage = TransferBufferUsage::Upload,
          .size = initial_metadata_capacity *
              static_cast<uint32_t>(sizeof(MeshletCullMetadata)),
      })},
      group_to_meshlet_dispatch_buffer{make_group_to_meshlet_dispatch_buffer(
          device, initial_group_capacity
      )},
      group_to_meshlet_dispatch_transfer_buffer{device.create_transfer_buffer(TransferBufferInfo{
          .usage = TransferBufferUsage::Upload,
          .size = initial_group_capacity * static_cast<uint32_t>(sizeof(uint32_t)),
      })} {}

auto SDL_GPUMeshletCullPass::cull(
    const SDL_GPUFactory& graphics_factory,
    CommandBuffer& command_buffer,
    const SDL_GPUInstanceBufferCache& instance_buffer_cache,
    gsl::span<const InstanceBatch> instance_batches,
    const InstanceCullLayout& phase_a_layout,
    const SDL_GPUInstanceCullPass& phase_a_cull_pass,
    const std::array<Vector4f, 6>& camera_frustum_planes,
    const Matrix4x4f& current_view_projection,
    const Texture& hiz_pyramid,
    const Sampler& hiz_sampler,
    uint32_t hiz_mip_levels
) -> MeshletCullLayout {
    auto layout = MeshletCullLayout{};
    layout.reserve(instance_batches.size());

    auto commands = std::vector<MeshletIndirectDrawCommand>{};
    auto metadata_entries = std::vector<MeshletCullMetadata>{};
    auto group_to_meshlet_dispatch = std::vector<uint32_t>{};
    auto batch_dispatch_infos = std::vector<BatchDispatchInfo>{};

    auto running_output_index_base = uint32_t{0};

    constexpr auto phase_a_command_size =
        static_cast<uint32_t>(sizeof(IndirectDrawCommand));

    for (auto batch_index = std::size_t{0}; batch_index < instance_batches.size();
         ++batch_index) {
        const auto& batch = instance_batches[batch_index];
        const auto meshes = graphics_factory.get_meshes(batch.renderable_id);
        const auto& phase_a_submesh_infos = phase_a_layout[batch_index];

        auto submesh_infos =
            std::vector<std::array<MeshletSubmeshCullInfo, max_lod_levels>>(
                meshes.size()
            );

        const auto batch_group_base =
            static_cast<uint32_t>(group_to_meshlet_dispatch.size());
        auto batch_group_count = uint32_t{0};

        for (auto mesh_index = std::size_t{0}; mesh_index < meshes.size();
             ++mesh_index) {
            const auto& mesh = meshes[mesh_index];
            const auto& phase_a_info = phase_a_submesh_infos[mesh_index];

            for (auto lod = std::size_t{0}; lod < max_lod_levels; ++lod) {
                const auto& meshlet_range = mesh.get_meshlet_range(lod);

                const auto output_command_index =
                    static_cast<uint32_t>(commands.size());
                const auto output_instance_base = running_output_index_base;
                // Worst case: every one of this batch's instances survives
                // Phase A for this LOD and every one of its meshlets
                // survives Phase B.
                running_output_index_base +=
                    batch.instance_count * meshlet_range.meshlet_count;

                commands.push_back(MeshletIndirectDrawCommand{
                    .num_vertices = meshlet_fixed_vertex_count,
                    .num_instances = 0U,
                    .first_vertex = 0U,
                    .first_instance = output_instance_base,
                });

                submesh_infos.at(mesh_index).at(lod) = MeshletSubmeshCullInfo{
                    .indirect_command_byte_offset = output_command_index *
                        static_cast<uint32_t>(sizeof(MeshletIndirectDrawCommand)),
                };

                // Nothing to cull for an empty meshlet range (e.g. a
                // degenerate zero-triangle submesh) - the command above
                // stays at num_instances = 0 forever, matching how Phase A
                // itself leaves unused LOD commands as permanent no-ops.
                if (meshlet_range.meshlet_count == 0U) {
                    continue;
                }

                const auto phase_a_byte_offset =
                    phase_a_info.indirect_command_byte_offsets.at(lod);
                const auto phase_a_command_index =
                    phase_a_byte_offset / phase_a_command_size;
                const auto phase_a_instance_base =
                    phase_a_info.instance_base_offsets.at(lod);

                const auto metadata_index =
                    static_cast<uint32_t>(metadata_entries.size());
                const auto first_group =
                    batch_group_base + batch_group_count;
                const auto worst_case_thread_count =
                    batch.instance_count * meshlet_range.meshlet_count;
                const auto group_count =
                    (worst_case_thread_count + threads_per_group - 1) /
                    threads_per_group;

                metadata_entries.push_back(MeshletCullMetadata{
                    .phase_a_command_index = phase_a_command_index,
                    .phase_a_instance_base = phase_a_instance_base,
                    .meshlet_first = meshlet_range.first_meshlet,
                    .meshlet_count = meshlet_range.meshlet_count,
                    .worst_case_instance_count = batch.instance_count,
                    .output_command_index = output_command_index,
                    .output_instance_base = output_instance_base,
                    .first_group = first_group,
                });

                group_to_meshlet_dispatch.insert(
                    group_to_meshlet_dispatch.end(), group_count, metadata_index
                );
                batch_group_count += group_count;
            }
        }

        layout.push_back(std::move(submesh_infos));

        if (batch_group_count > 0U) {
            batch_dispatch_infos.push_back(BatchDispatchInfo{
                .renderable_id = batch.renderable_id,
                .group_to_meshlet_dispatch_base = batch_group_base,
                .total_group_count = batch_group_count,
            });
        }
    }

    auto* const device = graphics_factory.get_gpu_device().get();

    const auto required_command_size = static_cast<uint32_t>(
        commands.size() * sizeof(MeshletIndirectDrawCommand)
    );
    if (indirect_command_buffer.get_size() < required_command_size) {
        indirect_command_buffer = make_indirect_command_buffer(
            *device, static_cast<uint32_t>(commands.size())
        );
    }
    if (indirect_command_transfer_buffer.get_size() < required_command_size) {
        indirect_command_transfer_buffer = device->create_transfer_buffer(TransferBufferInfo{
            .usage = TransferBufferUsage::Upload,
            .size = required_command_size,
        });
    }

    const auto required_visible_instance_size = running_output_index_base *
        static_cast<uint32_t>(sizeof(std::array<uint32_t, 2>));
    if (visible_meshlet_instances_buffer.get_size() <
        required_visible_instance_size) {
        visible_meshlet_instances_buffer = make_visible_meshlet_instances_buffer(
            *device, running_output_index_base
        );
    }

    const auto required_metadata_size = static_cast<uint32_t>(
        metadata_entries.size() * sizeof(MeshletCullMetadata)
    );
    if (meshlet_cull_metadata_buffer.get_size() < required_metadata_size) {
        meshlet_cull_metadata_buffer = make_meshlet_cull_metadata_buffer(
            *device, static_cast<uint32_t>(metadata_entries.size())
        );
    }
    if (meshlet_cull_metadata_transfer_buffer.get_size() < required_metadata_size) {
        meshlet_cull_metadata_transfer_buffer =
            device->create_transfer_buffer(TransferBufferInfo{
                .usage = TransferBufferUsage::Upload,
                .size = required_metadata_size,
            });
    }

    const auto required_group_size = static_cast<uint32_t>(
        group_to_meshlet_dispatch.size() * sizeof(uint32_t)
    );
    if (group_to_meshlet_dispatch_buffer.get_size() < required_group_size) {
        group_to_meshlet_dispatch_buffer = make_group_to_meshlet_dispatch_buffer(
            *device, static_cast<uint32_t>(group_to_meshlet_dispatch.size())
        );
    }
    if (group_to_meshlet_dispatch_transfer_buffer.get_size() < required_group_size) {
        group_to_meshlet_dispatch_transfer_buffer =
            device->create_transfer_buffer(TransferBufferInfo{
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

    if (!metadata_entries.empty()) {
        auto copy_pass = command_buffer.begin_copy_pass();
        const auto mapped = meshlet_cull_metadata_transfer_buffer.map(true);
        std::memcpy(
            mapped.data(), metadata_entries.data(), required_metadata_size
        );
        meshlet_cull_metadata_transfer_buffer.unmap();
        copy_pass.upload_to_buffer(
            meshlet_cull_metadata_transfer_buffer, 0, meshlet_cull_metadata_buffer,
            0, required_metadata_size, true
        );
    }

    if (!group_to_meshlet_dispatch.empty()) {
        auto copy_pass = command_buffer.begin_copy_pass();
        const auto mapped = group_to_meshlet_dispatch_transfer_buffer.map(true);
        std::memcpy(
            mapped.data(), group_to_meshlet_dispatch.data(), required_group_size
        );
        group_to_meshlet_dispatch_transfer_buffer.unmap();
        copy_pass.upload_to_buffer(
            group_to_meshlet_dispatch_transfer_buffer, 0,
            group_to_meshlet_dispatch_buffer, 0, required_group_size, true
        );
    }

    if (!batch_dispatch_infos.empty()) {
        const auto storage_bindings = std::array<StorageBufferReadWriteBinding, 2>{
            StorageBufferReadWriteBinding{
                .buffer = &indirect_command_buffer, .cycle = false
            },
            StorageBufferReadWriteBinding{
                .buffer = &visible_meshlet_instances_buffer, .cycle = false
            },
        };
        auto compute_pass = command_buffer.begin_compute_pass({}, storage_bindings);
        compute_pass.bind_compute_pipeline(meshlet_cull_pipeline);

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
            const auto read_only_bindings = std::array<const Buffer* const, 6>{
                &instance_models_buffer,
                &phase_a_cull_pass.get_visible_instance_indices_buffer(),
                &phase_a_cull_pass.get_indirect_command_buffer(),
                &meshlet_cull_metadata_buffer,
                &graphics_factory.get_meshlet_metadata_buffer(info.renderable_id),
                &group_to_meshlet_dispatch_buffer,
            };
            compute_pass.bind_storage_buffers(0, read_only_bindings);

            const auto params = MeshletCullParams{
                .frustum_planes = camera_frustum_planes,
                .current_view_projection = current_view_projection,
                .hiz_mip_levels = hiz_mip_levels,
                .group_to_meshlet_dispatch_base =
                    info.group_to_meshlet_dispatch_base,
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

auto SDL_GPUMeshletCullPass::get_indirect_command_buffer() const
    -> const Buffer& {
    return indirect_command_buffer;
}

auto SDL_GPUMeshletCullPass::get_visible_meshlet_instances_buffer() const
    -> const Buffer& {
    return visible_meshlet_instances_buffer;
}

}  // namespace Luminol::Graphics::SDL_GPU
