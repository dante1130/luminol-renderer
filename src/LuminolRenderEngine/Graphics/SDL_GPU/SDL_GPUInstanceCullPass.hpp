#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include <gsl/gsl>
#include <LuminolMaths/Vector.hpp>

#include <LuminolMaths/Matrix.hpp>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUComputePipeline.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUInstanceBatch.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTransferBuffer.hpp>

namespace Luminol::Graphics::SDL_GPU {

class GPUDevice;
class CommandBuffer;
class SDL_GPUFactory;
class SDL_GPUInstanceBufferCache;
class Texture;
class Sampler;

// Where one submesh's culled draw lives: indirect_command_byte_offset
// indexes into get_indirect_command_buffer() (pass straight to
// draw_indexed_primitives_indirect / SDL_GPUMesh::draw_indirect);
// instance_base_offset is this submesh's slice base in
// get_visible_instance_indices_buffer(), and must be pushed as the vertex
// shader's instance_base_offset uniform before that submesh's draw call.
struct SubmeshCullInfo {
    uint32_t indirect_command_byte_offset;
    uint32_t instance_base_offset;
};

// One entry per batch, one inner entry per submesh - same shape as
// BatchMeshBounds (SDL_GPUCullingUtils.hpp), indexed the same way by
// (batch_index, mesh_index).
using InstanceCullLayout = std::vector<std::vector<SubmeshCullInfo>>;

// GPU-driven per-instance frustum culling: one compute dispatch per BATCH
// (not per submesh) runs instance_cull.hlsl - one thread per instance,
// grouped so that every submesh in the batch is covered by the same
// dispatch via a group-index -> submesh-index lookup built each call. Each
// thread transforms its submesh's local AABB by its instance's model
// matrix, tests it against the camera frustum, and for survivors atomically
// compacts the instance's original index into visible_instance_indices
// while incrementing the matching IndirectDrawCommand's num_instances.
// Batching by batch (rather than one dispatch per submesh) matters because
// every submesh in a batch shares the same instance_models buffer, but
// different batches don't - so dispatch granularity can't go coarser than
// one per batch without also index into per-batch instance buffers.
// Per-frame CPU cost is proportional to submesh count, not instance count,
// unlike the CPU whole-batch AABB approach in SDL_GPUCullingUtils. Modeled
// on SDL_GPUClusterPass's compute-pass-before-render-pass structure.
class SDL_GPUInstanceCullPass {
public:
    explicit SDL_GPUInstanceCullPass(GPUDevice& device);

    // Must be called after instance data is uploaded (SDL_GPUInstanceBufferCache)
    // and before any render pass is opened on command_buffer this frame -
    // it opens its own copy pass and compute pass(es).
    // hiz_mip_levels = 0 disables the occlusion test (first frame after
    // construction/resize, when there is no valid previous-frame depth).
    [[nodiscard]] auto cull(
        const SDL_GPUFactory& graphics_factory,
        CommandBuffer& command_buffer,
        const SDL_GPUInstanceBufferCache& instance_buffer_cache,
        gsl::span<const InstanceBatch> instance_batches,
        const std::array<Maths::Vector4f, 6>& camera_frustum_planes,
        const Maths::Matrix4x4f& current_view_projection,
        const Texture& hiz_pyramid,
        const Sampler& hiz_sampler,
        uint32_t hiz_mip_levels
    ) -> InstanceCullLayout;

    [[nodiscard]] auto get_indirect_command_buffer() const -> const Buffer&;
    [[nodiscard]] auto get_visible_instance_indices_buffer() const
        -> const Buffer&;

private:
    ComputePipeline instance_cull_pipeline;

    Buffer indirect_command_buffer;
    TransferBuffer indirect_command_transfer_buffer;
    Buffer visible_instance_indices_buffer;

    // Per-submesh cull inputs (bounds, command_index, instance_base_offset,
    // instance_count, first_group) and a group-index -> submesh-index lookup,
    // both rebuilt and re-uploaded every cull() call. Together these let one
    // dispatch per batch cover every submesh in that batch (each thread group
    // looks up its own submesh via group_to_submesh), instead of one dispatch
    // per submesh - see the doc comment on cull().
    Buffer submesh_metadata_buffer;
    TransferBuffer submesh_metadata_transfer_buffer;
    Buffer group_to_submesh_buffer;
    TransferBuffer group_to_submesh_transfer_buffer;
};

}  // namespace Luminol::Graphics::SDL_GPU
