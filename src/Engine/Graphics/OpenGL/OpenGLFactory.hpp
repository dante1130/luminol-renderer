#pragma once

#include <Engine/Graphics/GraphicsFactory.hpp>

namespace Luminol::Graphics {

class OpenGLFactory : public GraphicsFactory {
public:
    [[nodiscard]] auto create_renderer(Window& window) const
        -> std::unique_ptr<Renderer> override;

    [[nodiscard]] auto create_mesh(
        gsl::span<const float> vertices,
        gsl::span<const uint32_t> indices,
        const TexturePaths& texture_paths
    ) const -> std::unique_ptr<Mesh> override;

    [[nodiscard]] auto create_model(const std::filesystem::path& model_path
    ) const -> std::unique_ptr<Model> override;

    [[nodiscard]] auto get_graphics_api() const -> GraphicsApi override;
};

}  // namespace Luminol::Graphics
