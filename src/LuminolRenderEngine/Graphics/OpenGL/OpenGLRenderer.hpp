#pragma once

#include <LuminolRenderEngine/Graphics/Renderer.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLShader.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLUniformBuffer.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLShaderStorageBuffer.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLUniforms.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLFrameBuffer.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLSkybox.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLModel.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLGBufferRenderPass.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLLightingRenderPass.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLColorRenderPass.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLDrawCall.hpp>

namespace Luminol::Graphics {

class OpenGLRenderer : public Renderer {
public:
    OpenGLRenderer(Window& window);

    [[nodiscard]] auto get_transform_uniform_buffer()
        -> OpenGLUniformBuffer<OpenGLUniforms::Transform>&;
    [[nodiscard]] auto get_instancing_model_matrix_buffer()
        -> OpenGLShaderStorageBuffer<glm::mat4>&;
    [[nodiscard]] auto get_instancing_color_buffer()
        -> OpenGLShaderStorageBuffer<glm::vec3>&;
    [[nodiscard]] auto get_view_matrix() const -> const glm::mat4&;
    [[nodiscard]] auto get_projection_matrix() const -> const glm::mat4&;
    [[nodiscard]] auto get_exposure() const -> float;

    auto set_view_matrix(const glm::mat4& view_matrix) -> void override;
    auto set_projection_matrix(const glm::mat4& projection_matrix)
        -> void override;

    auto set_exposure(float exposure) -> void override;

    auto clear_color(const glm::vec4& color) const -> void override;
    auto clear(BufferBit buffer_bit) const -> void override;
    auto queue_draw(const Renderable& renderable, const glm::mat4& model_matrix)
        -> void override;
    auto queue_draw_with_color(
        const Renderable& renderable,
        const glm::mat4& model_matrix,
        const glm::vec3& color
    ) -> void override;
    auto queue_draw_with_color_instanced(
        const Renderable& renderable,
        gsl::span<glm::mat4> model_matrices,
        gsl::span<glm::vec3> colors
    ) -> void override;
    auto draw() -> void override;

private:
    auto update_lights() -> void;
    auto get_framebuffer_resize_callback() -> Window::FramebufferSizeCallback;

    int32_t opengl_version;

    std::function<int32_t()> get_window_width;
    std::function<int32_t()> get_window_height;

    std::vector<DrawCall> draw_queue;
    std::vector<ColorDrawCall> color_draw_queue;
    std::vector<ColorDrawInstancedCall> instanced_color_draw_queue;

    OpenGLGBufferRenderPass gbuffer_render_pass;
    OpenGLLightingRenderPass lighting_render_pass;
    OpenGLColorRenderPass color_render_pass;

    OpenGLUniformBuffer<OpenGLUniforms::Transform> transform_uniform_buffer;
    OpenGLUniformBuffer<OpenGLUniforms::Light> light_uniform_buffer;
    OpenGLShaderStorageBuffer<glm::mat4> instancing_model_matrix_buffer;
    OpenGLShaderStorageBuffer<glm::vec3> instancing_color_buffer;

    OpenGLSkybox skybox;

    float exposure = 1.0f;

    glm::mat4 view_matrix;
    glm::mat4 projection_matrix;
};

}  // namespace Luminol::Graphics
