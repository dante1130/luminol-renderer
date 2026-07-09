#pragma once

#include <memory>
#include <filesystem>

#include <gsl/gsl>

#include <LuminolRenderEngine/Graphics/GraphicsApi.hpp>
#include <LuminolRenderEngine/Graphics/Renderer.hpp>
#include <LuminolRenderEngine/Graphics/RenderableManager.hpp>
#include <LuminolRenderEngine/Graphics/TexturePaths.hpp>
#include <LuminolRenderEngine/Window/Window.hpp>

namespace Luminol::Graphics {

class GraphicsFactory : public std::enable_shared_from_this<GraphicsFactory> {
public:
    GraphicsFactory() = default;
    virtual ~GraphicsFactory() = default;
    GraphicsFactory(const GraphicsFactory&) = default;
    GraphicsFactory(GraphicsFactory&&) = default;
    auto operator=(const GraphicsFactory&) -> GraphicsFactory& = default;
    auto operator=(GraphicsFactory&&) -> GraphicsFactory& = default;

    [[nodiscard]] static auto create(GraphicsApi api)
        -> std::shared_ptr<GraphicsFactory>;

    [[nodiscard]] virtual auto create_renderer(Window& window)
        -> std::unique_ptr<Renderer> = 0;

    [[nodiscard]] virtual auto create_mesh(
        gsl::span<const float> vertices,
        gsl::span<const uint32_t> indices,
        const TexturePaths& texture_paths
    ) -> RenderableId = 0;

    [[nodiscard]] virtual auto create_model(
        const std::filesystem::path& model_path
    ) -> RenderableId = 0;

    virtual auto remove_renderable(RenderableId renderable_id) -> void = 0;

    [[nodiscard]] virtual auto create_font(
        const std::filesystem::path& font_path, float point_size
    ) -> FontId = 0;

    [[nodiscard]] virtual auto get_graphics_api() const -> GraphicsApi = 0;
};

}  // namespace Luminol::Graphics
