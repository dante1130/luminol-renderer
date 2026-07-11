#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include <gsl/gsl>
#include <LuminolMaths/Vector.hpp>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUComputePipeline.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUInstanceBatch.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTransferBuffer.hpp>

namespace Luminol::Graphics::SDL_GPU {

class GPUDevice;
class CommandBuffer;
class SDL_GPUFactory;
class SDL_GPUInstanceBufferCache;

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

// GPU-driven per-instance frustum culling: for every submesh across every
// queued batch, dispatches one compute pass (instance_cull.hlsl, one thread
// per instance) that transforms the submesh's local AABB by each instance's
// model matrix, tests it against the camera frustum, and for survivors
// atomically compacts the instance's original index into
// visible_instance_indices while incrementing the matching
// IndirectDrawCommand's num_instances - so per-frame CPU cost is
// proportional to submesh count, not instance count, unlike the CPU
// whole-batch AABB approach in SDL_GPUCullingUtils. Modeled on
// SDL_GPUClusterPass's compute-pass-before-render-pass structure.
class SDL_GPUInstanceCullPass {
public:
    explicit SDL_GPUInstanceCullPass(GPUDevice& device);

    // Must be called after instance data is uploaded (SDL_GPUInstanceBufferCache)
    // and before any render pass is opened on command_buffer this frame -
    // it opens its own copy pass and compute pass(es).
    [[nodiscard]] auto cull(
        const SDL_GPUFactory& graphics_factory,
        CommandBuffer& command_buffer,
        const SDL_GPUInstanceBufferCache& instance_buffer_cache,
        gsl::span<const InstanceBatch> instance_batches,
        const std::array<Maths::Vector4f, 6>& camera_frustum_planes
    ) -> InstanceCullLayout;

    [[nodiscard]] auto get_indirect_command_buffer() const -> const Buffer&;
    [[nodiscard]] auto get_visible_instance_indices_buffer() const
        -> const Buffer&;

private:
    ComputePipeline instance_cull_pipeline;

    Buffer indirect_command_buffer;
    TransferBuffer indirect_command_transfer_buffer;
    Buffer visible_instance_indices_buffer;
};

}  // namespace Luminol::Graphics::SDL_GPU
