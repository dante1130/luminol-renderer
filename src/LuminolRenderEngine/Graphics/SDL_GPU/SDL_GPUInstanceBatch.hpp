#pragma once

#include <cstdint>

#include <LuminolRenderEngine/Graphics/RenderableManager.hpp>

namespace Luminol::Graphics::SDL_GPU {

struct InstanceBatch {
    RenderableId renderable_id;
    uint32_t instance_count;
};

}  // namespace Luminol::Graphics::SDL_GPU
