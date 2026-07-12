#include "Renderer.hpp"

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUFactory.hpp>

namespace Luminol::Graphics {

Renderer::Renderer(std::shared_ptr<SDL_GPU::SDL_GPUFactory> graphics_factory)
    : graphics_factory{std::move(graphics_factory)} {}

auto Renderer::create_renderable(
    gsl::span<const float> vertices,
    gsl::span<const uint32_t> indices,
    const TexturePaths& texture_paths
) -> RenderableId {
    return this->graphics_factory->create_mesh(vertices, indices, texture_paths);
}

auto Renderer::create_renderable(const std::filesystem::path& model_path)
    -> RenderableId {
    return this->graphics_factory->create_model(model_path);
}

auto Renderer::remove_renderable(RenderableId renderable_id) -> void {
    this->graphics_factory->remove_renderable(renderable_id);
}

auto Renderer::create_font(
    const std::filesystem::path& font_path, float point_size
) -> FontId {
    return this->graphics_factory->create_font(font_path, point_size);
}

auto Renderer::get_light_manager() const -> const LightManager& {
    return this->light_manager;
}

auto Renderer::get_light_manager() -> LightManager& {
    return this->light_manager;
}

}  // namespace Luminol::Graphics
