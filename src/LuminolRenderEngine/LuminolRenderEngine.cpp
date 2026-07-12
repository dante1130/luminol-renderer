#include "LuminolRenderEngine.hpp"

#include <LuminolRenderEngine/Graphics/Light.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUFactory.hpp>

namespace Luminol {

RenderEngine::RenderEngine(const Properties& properties)
    : window(properties.width, properties.height, properties.title),
      renderer(std::make_shared<Graphics::SDL_GPU::SDL_GPUFactory>(
                   properties.msaa_sample_count
               )
                   ->create_renderer(this->window)) {}

auto RenderEngine::get_window() const -> const Window& { return this->window; }

auto RenderEngine::get_window() -> Window& { return this->window; }

auto RenderEngine::get_renderer() const -> const Graphics::Renderer& {
    return *this->renderer;
}

auto RenderEngine::get_renderer() -> Graphics::Renderer& {
    return *this->renderer;
}

}  // namespace Luminol
