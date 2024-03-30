#pragma once

#include <Engine/Graphics/Renderer.hpp>
#include <Engine/Graphics/OpenGL/OpenGLShader.hpp>
#include <Engine/Graphics/OpenGL/OpenGLUniformBuffer.hpp>

namespace Luminol::Graphics {

struct Transform {
    glm::mat4 model = {1.0f};
};

class OpenGLRenderer : public Renderer {
public:
    OpenGLRenderer(const Window& window);

    auto clear_color(const glm::vec4& color) const -> void override;
    auto clear(BufferBit buffer_bit) const -> void override;
    auto draw(
        const RenderCommand& render_command, const glm::mat4& model_matrix
    ) const -> void override;

private:
    std::unique_ptr<OpenGLShader> shader = {nullptr};
    std::unique_ptr<OpenGLUniformBuffer<Transform>> transform_uniform_buffer = {
        nullptr
    };
};

}  // namespace Luminol::Graphics
