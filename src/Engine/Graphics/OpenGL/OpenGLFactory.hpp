#pragma once

#include <Engine/Graphics/GraphicsFactory.hpp>

namespace Luminol::Graphics {

class OpenGLFactory : public GraphicsFactory {
public:
    [[nodiscard]] auto create_renderer(const Window& window)
        -> std::unique_ptr<Renderer> override;
    [[nodiscard]] auto get_graphics_api() const -> GraphicsApi override;
};

}  // namespace Luminol::Graphics
