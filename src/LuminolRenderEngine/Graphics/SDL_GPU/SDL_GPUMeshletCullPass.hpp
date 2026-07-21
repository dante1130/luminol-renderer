#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include <gsl/gsl>
#include <LuminolMaths/Matrix.hpp>
#include <LuminolMaths/Vector.hpp>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUComputePipeline.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUInstanceBatch.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUInstanceCullPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTransferBuffer.hpp>

namespace Luminol::Graphics::SDL_GPU {

class GPUDevice;
class CommandBuffer;
class SDL_GPUFactory;
class SDL_GPUInstanceBufferCache;
class Texture;
class Sampler;

// Where one (submesh, LOD)'s meshlet-culled draw lives: a single
// non-indexed indirect draw command (SDL_GPUIndirectDrawCommand layout, not
// SDL_GPUIndexedIndirectDrawCommand - see meshlet_cull.hlsl's
// IndirectDrawCommand) whose num_instances is the count of surviving
// (instance, meshlet) pairs, written by SDL_GPUMeshletCullPass::cull.
struct MeshletSubmeshCullInfo {
    uint32_t indirect_command_byte_offset;
};

// One entry per batch, one inner entry per submesh - same shape/indexing as
// InstanceCullLayout (SDL_GPUInstanceCullPass.hpp), by (batch_index,
// mesh_index). Each submesh's max_lod_levels indirect commands are
// contiguous in get_indirect_command_buffer() in LOD order, mirroring
// SDL_GPUInstanceCullPass's layout - callers that need one LOD's command
// index it directly by offsetting from the first.
using MeshletCullLayout = std::vector<std::vector<
    std::array<MeshletSubmeshCullInfo, max_lod_levels>>>;

// Phase B of meshlet-level GPU culling, main color pass only (see
// meshlet_cull.hlsl's file comment for the full design). Consumes
// SDL_GPUInstanceCullPass's (Phase A) output unchanged and read-only: for
// each of Phase A's surviving (submesh, LOD) instances, further culls at
// meshlet (~64-triangle cluster) granularity, one thread per candidate
// (instance, meshlet) pair. Dispatch is sized on the CPU-known worst case
// (batch.instance_count x meshlets-in-that-LOD) with GPU-side early-exit
// against Phase A's actual surviving instance count, the same
// worst-case-dispatch-plus-early-exit pattern instance_cull.hlsl already
// uses - not a new mechanism. Output draw count is unchanged from Phase A's
// shape (one non-indexed indirect draw per (submesh, LOD)) - a naive
// one-command-per-surviving-meshlet design was deliberately rejected (see
// meshlet_cull.hlsl's file comment) because SDL_GPU's multiDrawIndirect is
// an optional Vulkan feature and a large multi-draw count silently degrades
// into one real draw call per entry in a CPU loop on hardware without it.
class SDL_GPUMeshletCullPass {
public:
    explicit SDL_GPUMeshletCullPass(GPUDevice& device);

    // Must be called after phase_a_cull_pass.cull() (same command_buffer,
    // same frame, before any render pass is opened) - opens its own copy
    // pass and compute pass(es). phase_a_layout is the InstanceCullLayout
    // phase_a_cull_pass.cull() returned this same call.
    [[nodiscard]] auto cull(
        const SDL_GPUFactory& graphics_factory,
        CommandBuffer& command_buffer,
        const SDL_GPUInstanceBufferCache& instance_buffer_cache,
        gsl::span<const InstanceBatch> instance_batches,
        const InstanceCullLayout& phase_a_layout,
        const SDL_GPUInstanceCullPass& phase_a_cull_pass,
        const std::array<Maths::Vector4f, 6>& camera_frustum_planes,
        const Maths::Matrix4x4f& current_view_projection,
        const Texture& hiz_pyramid,
        const Sampler& hiz_sampler,
        uint32_t hiz_mip_levels,
        const Maths::Vector3f& camera_position
    ) -> MeshletCullLayout;

    [[nodiscard]] auto get_indirect_command_buffer() const -> const Buffer&;
    [[nodiscard]] auto get_visible_meshlet_instances_buffer() const
        -> const Buffer&;

private:
    ComputePipeline meshlet_cull_pipeline;

    Buffer indirect_command_buffer;
    TransferBuffer indirect_command_transfer_buffer;
    // x = original instance index, y = meshlet index - see
    // meshlet_cull.hlsl's visible_meshlet_instances (RWStructuredBuffer<uint2>).
    Buffer visible_meshlet_instances_buffer;

    // Per-(submesh,LOD) cull inputs (mirrors instance_cull.hlsl's
    // submesh_metadata/group_to_submesh pattern, one extra dimension for
    // meshlets) and a group-index -> metadata-index lookup, both rebuilt
    // and re-uploaded every cull() call.
    Buffer meshlet_cull_metadata_buffer;
    TransferBuffer meshlet_cull_metadata_transfer_buffer;
    Buffer group_to_meshlet_dispatch_buffer;
    TransferBuffer group_to_meshlet_dispatch_transfer_buffer;
};

}  // namespace Luminol::Graphics::SDL_GPU
