#include "Renderer.hpp"

#include <LuminolRenderEngine/Graphics/RenderableManager.hpp>

namespace Luminol::Graphics {

Renderer::Renderer(GraphicsApi graphics_api)
    : renderable_manager{graphics_api} {}

auto Renderer::get_renderable_manager() const -> const RenderableManager& {
    return this->renderable_manager;
}

auto Renderer::get_renderable_manager() -> RenderableManager& {
    return this->renderable_manager;
}

auto Renderer::get_light_manager() const -> const LightManager& {
    return this->light_manager;
}

auto Renderer::get_light_manager() -> LightManager& {
    return this->light_manager;
}

}  // namespace Luminol::Graphics
