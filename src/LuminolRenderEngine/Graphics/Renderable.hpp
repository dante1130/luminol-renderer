#pragma once

#include <cstdint>

namespace Luminol::Graphics::SDL_GPU {
class RenderPass;
}  // namespace Luminol::Graphics::SDL_GPU

namespace Luminol::Graphics {

using RenderableId = uint32_t;

class Renderable {
public:
    Renderable() = default;
    virtual ~Renderable() = default;
    Renderable(const Renderable&) = delete;
    Renderable(Renderable&&) = default;
    auto operator=(const Renderable&) -> Renderable& = delete;
    auto operator=(Renderable&&) -> Renderable& = default;

    virtual auto draw() const -> void = 0;
    virtual auto draw_instanced(int32_t instance_count) const -> void = 0;

    virtual auto draw(SDL_GPU::RenderPass& /*sdl_gpu_pass*/) const -> void {}
    virtual auto draw_instanced(
        int32_t /*instance_count*/, SDL_GPU::RenderPass& /*sdl_gpu_pass*/
    ) const -> void {}
};

}  // namespace Luminol::Graphics
