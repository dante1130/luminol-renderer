#pragma once

#include <Engine/Graphics/GraphicsFactory.hpp>

namespace Luminol::Graphics {

class OpenGLFactory : public GraphicsFactory {
public:
    [[nodiscard]] auto create_renderer(const Window& window)
        -> std::unique_ptr<Renderer> override;
    [[nodiscard]] auto create_shader(const ShaderPaths& paths)
        -> std::unique_ptr<Shader> override;
    [[nodiscard]] auto create_mesh(gsl::span<const float> vertices)
        -> std::unique_ptr<Mesh> override;
    [[nodiscard]] auto get_graphics_api() const -> GraphicsApi override;
};

}  // namespace Luminol::Graphics
