#pragma once

#include <memory>
#include <filesystem>

#include <gsl/gsl>

#include <Engine/Graphics/GraphicsApi.hpp>
#include <Engine/Graphics/Renderer.hpp>
#include <Engine/Graphics/TexturePaths.hpp>
#include <Engine/Graphics/Mesh.hpp>
#include <Engine/Graphics/Model.hpp>
#include <Engine/Window/Window.hpp>

namespace Luminol::Graphics {

class GraphicsFactory {
public:
    GraphicsFactory() = default;
    virtual ~GraphicsFactory() = default;
    GraphicsFactory(const GraphicsFactory&) = default;
    GraphicsFactory(GraphicsFactory&&) = delete;
    auto operator=(const GraphicsFactory&) -> GraphicsFactory& = default;
    auto operator=(GraphicsFactory&&) -> GraphicsFactory& = delete;

    [[nodiscard]] static auto create(GraphicsApi api)
        -> std::unique_ptr<GraphicsFactory>;

    [[nodiscard]] virtual auto create_renderer(Window& window)
        -> std::unique_ptr<Renderer> = 0;

    [[nodiscard]] virtual auto create_mesh(
        gsl::span<const float> vertices,
        gsl::span<const uint32_t> indices,
        const TexturePaths& texture_paths
    ) -> std::unique_ptr<Mesh> = 0;

    [[nodiscard]] virtual auto create_model(
        const std::filesystem::path& model_path
    ) -> std::unique_ptr<Model> = 0;

    [[nodiscard]] virtual auto get_graphics_api() const -> GraphicsApi = 0;
};

}  // namespace Luminol::Graphics
