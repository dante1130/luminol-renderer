#pragma once

#include <Engine/Graphics/Renderer.hpp>
#include <Engine/Graphics/OpenGL/OpenGLShader.hpp>
#include <Engine/Graphics/OpenGL/OpenGLUniformBuffer.hpp>
#include <Engine/Graphics/OpenGL/OpenGLUniforms.hpp>

namespace Luminol::Graphics {

class OpenGLRenderer : public Renderer {
public:
    using DrawCall = std::function<void()>;

    OpenGLRenderer(Window& window);

    auto set_view_matrix(const glm::mat4& view_matrix) -> void override;
    auto set_projection_matrix(const glm::mat4& projection_matrix)
        -> void override;

    auto clear_color(const glm::vec4& color) const -> void override;
    auto clear(BufferBit buffer_bit) const -> void override;
    auto update_light(const Light& light) -> void override;
    auto queue_draw_with_phong(
        const RenderCommand& render_command, const glm::mat4& model_matrix
    ) -> void override;
    auto queue_draw_with_color(
        const RenderCommand& render_command,
        const glm::mat4& model_matrix,
        const glm::vec3& color
    ) -> void override;
    auto draw() -> void override;

private:
    std::vector<DrawCall> draw_queue = {};

    std::unique_ptr<OpenGLShader> color_shader = {nullptr};
    std::unique_ptr<OpenGLShader> phong_shader = {nullptr};

    std::unique_ptr<OpenGLUniformBuffer<OpenGLUniforms::Transform>>
        transform_uniform_buffer = {nullptr};
    std::unique_ptr<OpenGLUniformBuffer<OpenGLUniforms::Light>>
        light_uniform_buffer = {nullptr};

    glm::mat4 view_matrix = {1.0f};
    glm::mat4 projection_matrix = {1.0f};
};

}  // namespace Luminol::Graphics
