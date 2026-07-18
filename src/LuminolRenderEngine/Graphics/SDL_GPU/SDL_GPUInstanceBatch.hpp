#pragma once

#include <cstdint>
#include <vector>

#include <LuminolMaths/Matrix.hpp>

#include <LuminolRenderEngine/Graphics/RenderableManager.hpp>

namespace Luminol::Graphics::SDL_GPU {

struct InstanceBatch {
    RenderableId renderable_id;
    uint32_t instance_count;
};

// Per-RenderableId queued-draw state for the current frame, indexed directly
// by id (allocated monotonically, never reused - see
// RenderableManager::get_free_renderable_id) instead of an unordered_map, so
// every render pass's per-batch lookup is a direct index instead of a hash
// probe. registered distinguishes "never queued" from "queued but currently
// empty" - needed because the dense vectors must stay index-valid for every
// id up to the highest ever queued, not just the ones actually in use this
// frame.
struct QueuedDraws {
    std::vector<std::vector<Maths::Matrix4x4f>> model_matrices;
    std::vector<std::uint8_t> registered;
    std::vector<std::uint8_t> is_static;
    std::vector<std::uint8_t> pending_static_upload;
};

// Grows every vector in queued_draws together so renderable_id is always a
// valid index into all four afterward.
inline auto ensure_capacity(QueuedDraws& queued_draws, RenderableId renderable_id)
    -> void {
    if (renderable_id < queued_draws.model_matrices.size()) {
        return;
    }

    const auto new_size = renderable_id + 1;
    queued_draws.model_matrices.resize(new_size);
    queued_draws.registered.resize(new_size, 0);
    queued_draws.is_static.resize(new_size, 0);
    queued_draws.pending_static_upload.resize(new_size, 0);
}

}  // namespace Luminol::Graphics::SDL_GPU
