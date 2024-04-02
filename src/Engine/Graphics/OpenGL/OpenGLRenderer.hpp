#pragma once

#include <Engine/Graphics/Renderer.hpp>
#include <Engine/Graphics/OpenGL/OpenGLShader.hpp>
#include <Engine/Graphics/OpenGL/OpenGLUniformBuffer.hpp>
#include <Engine/Graphics/OpenGL/OpenGLUniforms.hpp>

namespace Luminol::Graphics {

class OpenGLRenderer : public Renderer {
public:
    OpenGLRenderer(Window& window);

    auto set_view_matrix(const glm::mat4& view_matrix) -> void override;
    auto set_projection_matrix(const glm::mat4& projection_matrix)
        -> void override;

    auto clear_color(const glm::vec4& color) const -> void override;
    auto clear(BufferBit buffer_bit) const -> void override;
    auto update_light(const Light& light) -> void override;
    auto draw(
        const RenderCommand& render_command,
        const glm::mat4& model_matrix,
        ShaderType shader_type
    ) const -> void override;

private:
    auto bind_shader(ShaderType shader_type) const -> void;

    std::unique_ptr<OpenGLShader> phong_shader = {nullptr};
    std::unique_ptr<OpenGLUniformBuffer<OpenGLUniforms::Transform>>
        transform_uniform_buffer = {nullptr};
    std::unique_ptr<OpenGLUniformBuffer<OpenGLUniforms::Light>>
        light_uniform_buffer = {nullptr};

    glm::mat4 view_matrix = {1.0f};
    glm::mat4 projection_matrix = {1.0f};
};

}  // namespace Luminol::Graphics
