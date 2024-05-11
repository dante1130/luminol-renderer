#include "LuminolRenderEngine.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <LuminolRenderEngine/Graphics/Light.hpp>

namespace Luminol {

RenderEngine::RenderEngine(const Properties& properties)
    : window(properties.width, properties.height, properties.title),
      renderer(Graphics::GraphicsFactory::create(properties.graphics_api)
                   ->create_renderer(this->window, properties.graphics_api)) {}

auto RenderEngine::get_window() const -> const Window& { return this->window; }

auto RenderEngine::get_window() -> Window& { return this->window; }

auto RenderEngine::get_renderer() const -> const Graphics::Renderer& {
    return *this->renderer;
}

auto RenderEngine::get_renderer() -> Graphics::Renderer& {
    return *this->renderer;
}

}  // namespace Luminol
