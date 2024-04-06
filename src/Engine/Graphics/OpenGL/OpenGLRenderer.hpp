#pragma once

#include <Engine/Graphics/Renderer.hpp>
#include <Engine/Graphics/OpenGL/OpenGLShader.hpp>
#include <Engine/Graphics/OpenGL/OpenGLUniformBuffer.hpp>
#include <Engine/Graphics/OpenGL/OpenGLUniforms.hpp>
#include <Engine/Graphics/OpenGL/OpenGLFrameBuffer.hpp>
#include <Engine/Graphics/OpenGL/OpenGLSkybox.hpp>
#include <Engine/Graphics/OpenGL/OpenGLModel.hpp>

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
        const RenderCommand& render_command,
        const glm::mat4& model_matrix,
        const Material& material
    ) -> void override;
    auto queue_draw_with_cell_shading(
        const RenderCommand& render_command,
        const glm::mat4& model_matrix,
        const Material& material,
        float cell_shading_levels
    ) -> void override;
    auto queue_draw_with_color(
        const RenderCommand& render_command,
        const glm::mat4& model_matrix,
        const glm::vec3& color
    ) -> void override;
    auto draw() -> void override;

private:
    auto draw_skybox() -> void;

    std::function<int32_t()> get_window_width;
    std::function<int32_t()> get_window_height;

    std::vector<DrawCall> draw_queue = {};

    std::unique_ptr<OpenGLShader> color_shader = {nullptr};
    std::unique_ptr<OpenGLShader> phong_shader = {nullptr};
    std::unique_ptr<OpenGLShader> skybox_shader = {nullptr};

    std::unique_ptr<OpenGLFrameBuffer> low_res_frame_buffer = {nullptr};

    std::unique_ptr<OpenGLUniformBuffer<OpenGLUniforms::Transform>>
        transform_uniform_buffer = {nullptr};
    std::unique_ptr<OpenGLUniformBuffer<OpenGLUniforms::Light>>
        light_uniform_buffer = {nullptr};

    std::unique_ptr<OpenGLSkybox> skybox = {nullptr};
    std::unique_ptr<OpenGLModel> cube = {nullptr};

    glm::mat4 view_matrix = {1.0f};
    glm::mat4 projection_matrix = {1.0f};
};

}  // namespace Luminol::Graphics
