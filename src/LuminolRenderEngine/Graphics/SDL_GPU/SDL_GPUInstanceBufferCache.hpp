#pragma once

#include <optional>
#include <vector>

#include <gsl/gsl>
#include <LuminolMaths/Matrix.hpp>

#include <LuminolRenderEngine/Graphics/RenderableManager.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTransferBuffer.hpp>

namespace Luminol::Graphics::SDL_GPU {

class GPUDevice;
class CopyPass;

// Owns a persistent, per-renderable GPU storage buffer (plus its staging
// transfer buffer) used to upload per-instance model matrices. Buffers are
// grown in place as larger batches are seen, never recreated on every frame.
class SDL_GPUInstanceBufferCache {
public:
    auto upload(
        GPUDevice& device,
        CopyPass& copy_pass,
        RenderableId renderable_id,
        gsl::span<const Maths::Matrix4x4f> model_matrices
    ) -> const Buffer&;

    [[nodiscard]] auto get(RenderableId renderable_id) const -> const Buffer&;

    // Shared identity mapping buffer (element i == i), grown lazily in
    // upload() to cover the largest instance count seen so far. pbr_vert.hlsl
    // always indexes instance_models through a visible_instance_indices
    // indirection (see SDL_GPUInstanceCullPass, which populates a *culled*
    // mapping); passes that draw every instance uncompacted (the shadow
    // passes) bind this identity buffer instead, making that indirection a
    // no-op.
    [[nodiscard]] auto get_identity_indices_buffer() const -> const Buffer&;

private:
    // Indexed directly by RenderableId (allocated monotonically, never
    // reused), so upload()/get() are a direct index instead of a hash probe.
    // nullopt until the first upload() call for that id.
    std::vector<std::optional<Buffer>> instance_buffers;
    std::vector<std::optional<TransferBuffer>> instance_transfer_buffers;

    std::optional<Buffer> identity_indices_buffer;
    std::optional<TransferBuffer> identity_indices_transfer_buffer;
};

}  // namespace Luminol::Graphics::SDL_GPU
