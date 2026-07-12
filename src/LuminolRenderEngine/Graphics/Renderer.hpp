#pragma once

#include <LuminolMaths/Vector.hpp>
#include <LuminolMaths/Matrix.hpp>

#include <filesystem>
#include <memory>
#include <string_view>

#include <gsl/gsl>

#include <LuminolRenderEngine/Window/Window.hpp>
#include <LuminolRenderEngine/Graphics/LightManager.hpp>
#include <LuminolRenderEngine/Graphics/RenderableManager.hpp>
#include <LuminolRenderEngine/Graphics/TexturePaths.hpp>

namespace Luminol::Graphics::SDL_GPU {
class SDL_GPUFactory;
}  // namespace Luminol::Graphics::SDL_GPU

namespace Luminol::Graphics {

class Renderer {
public:
    Renderer(std::shared_ptr<SDL_GPU::SDL_GPUFactory> graphics_factory);
    virtual ~Renderer() = default;
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

    virtual auto set_view_matrix(const Maths::Matrix4x4f& view_matrix)
        -> void = 0;
    virtual auto set_projection_matrix(
        const Maths::Matrix4x4f& projection_matrix
    ) -> void = 0;

    virtual auto set_exposure(float exposure) -> void = 0;

    virtual auto clear_color(const Maths::Vector4f& color) const -> void = 0;

    virtual auto queue_draw(
        RenderableId renderable_id, const Maths::Matrix4x4f& model_matrix
    ) -> void = 0;

    virtual auto queue_draw_instanced(
        RenderableId renderable_id,
        gsl::span<const Maths::Matrix4x4f> model_matrices
    ) -> void = 0;

    virtual auto queue_draw_text(
        FontId font_id,
        std::string_view text,
        const Maths::Vector2f& position,
        const Maths::Vector4f& color
    ) -> void = 0;

    virtual auto draw() -> void = 0;

    // Debug-only hooks for investigating occlusion culling; no-op by default
    // since not every backend implements occlusion culling.
    virtual auto set_debug_disable_occlusion_culling(bool disabled) -> void {
        static_cast<void>(disabled);
    }
    virtual auto debug_log_visible_instance_count() -> void {}
    virtual auto set_debug_visualize_hiz(bool enabled) -> void {
        static_cast<void>(enabled);
    }

private:
    std::shared_ptr<SDL_GPU::SDL_GPUFactory> graphics_factory;
    LightManager light_manager;
};

}  // namespace Luminol::Graphics
