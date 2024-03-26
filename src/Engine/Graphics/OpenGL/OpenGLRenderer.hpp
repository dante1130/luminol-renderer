#pragma once

#include <Engine/Graphics/Renderer.hpp>

namespace Luminol::Graphics {

class OpenGLRenderer : public Renderer {
public:
    OpenGLRenderer(const Window& window);

    auto clear_color(const glm::vec4& color) const -> void override;
    auto clear(BufferBit buffer_bit) const -> void override;
};

}  // namespace Luminol::Graphics
