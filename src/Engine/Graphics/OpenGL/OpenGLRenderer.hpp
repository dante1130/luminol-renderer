#pragma once

#include <Engine/Graphics/Renderer.hpp>
#include <Engine/Graphics/OpenGL/OpenGLShader.hpp>

namespace Luminol::Graphics {

class OpenGLRenderer : public Renderer {
public:
    OpenGLRenderer(const Window& window);

    auto clear_color(const glm::vec4& color) const -> void override;
    auto clear(BufferBit buffer_bit) const -> void override;
    auto draw(const RenderCommand& render_command) const -> void override;

private:
    std::unique_ptr<OpenGLShader> shader = {nullptr};
};

}  // namespace Luminol::Graphics
