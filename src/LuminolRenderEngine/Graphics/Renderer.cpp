#include "Renderer.hpp"

#include <LuminolRenderEngine/Graphics/RenderableManager.hpp>

namespace Luminol::Graphics {

Renderer::Renderer(
    GraphicsApi /*graphics_api*/,
    std::shared_ptr<GraphicsFactory> graphics_factory
)
    : renderable_manager{std::move(graphics_factory)} {}

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
