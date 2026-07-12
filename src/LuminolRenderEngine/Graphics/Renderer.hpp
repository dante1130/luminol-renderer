#pragma once

#include <filesystem>
#include <memory>

#include <gsl/gsl>

#include <LuminolRenderEngine/Graphics/LightManager.hpp>
#include <LuminolRenderEngine/Graphics/RenderableManager.hpp>
#include <LuminolRenderEngine/Graphics/TexturePaths.hpp>

namespace Luminol::Graphics::SDL_GPU {
class SDL_GPUFactory;
}  // namespace Luminol::Graphics::SDL_GPU

namespace Luminol::Graphics {

// Shared, backend-agnostic renderable/light bookkeeping. Not a polymorphic
// interface - SDL_GPURenderer is the only backend and is always held and
// destroyed through its own concrete type, so these methods and the
// destructor are deliberately non-virtual.
class Renderer {
public:
    Renderer(std::shared_ptr<SDL_GPU::SDL_GPUFactory> graphics_factory);
    ~Renderer() = default;
    Renderer(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    auto operator=(const Renderer&) -> Renderer& = delete;
    auto operator=(Renderer&&) -> Renderer& = delete;

    [[nodiscard]] auto create_renderable(
        gsl::span<const float> vertices,
        gsl::span<const uint32_t> indices,
        const TexturePaths& texture_paths
    ) -> RenderableId;

    [[nodiscard]] auto create_renderable(
        const std::filesystem::path& model_path
    ) -> RenderableId;

    auto remove_renderable(RenderableId renderable_id) -> void;

    [[nodiscard]] auto create_font(
        const std::filesystem::path& font_path, float point_size
    ) -> FontId;

    [[nodiscard]] auto get_light_manager() const -> const LightManager&;
    [[nodiscard]] auto get_light_manager() -> LightManager&;

private:
    std::shared_ptr<SDL_GPU::SDL_GPUFactory> graphics_factory;
    LightManager light_manager;
};

}  // namespace Luminol::Graphics
