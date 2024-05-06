#include "LuminolRenderEngine.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <LuminolRenderEngine/Graphics/Light.hpp>

namespace Luminol {

RenderEngine::RenderEngine(const Properties& properties)
    : window(properties.width, properties.height, properties.title),
      graphics_factory(Graphics::GraphicsFactory::create(properties.graphics_api
      )),
      renderer(this->graphics_factory->create_renderer(this->window)) {}

auto RenderEngine::get_window() const -> const Window& { return this->window; }

auto RenderEngine::get_window() -> Window& { return this->window; }

auto RenderEngine::get_graphics_factory() const
    -> const Graphics::GraphicsFactory& {
    return *this->graphics_factory;
}

auto RenderEngine::get_renderer() const -> const Graphics::Renderer& {
    return *this->renderer;
}

auto RenderEngine::get_renderer() -> Graphics::Renderer& {
    return *this->renderer;
}

}  // namespace Luminol
