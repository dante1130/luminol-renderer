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
    OpenGLRenderer(Window& window);

    auto set_view_matrix(const glm::mat4& view_matrix) -> void override;
    auto set_projection_matrix(const glm::mat4& projection_matrix)
        -> void override;

    auto set_exposure(float exposure) -> void override;

    auto clear_color(const glm::vec4& color) const -> void override;
    auto clear(BufferBit buffer_bit) const -> void override;
    auto queue_draw(const Renderable& renderable, const glm::mat4& model_matrix)
        -> void override;
    auto queue_draw_with_cell_shading(
        const Renderable& renderable,
        const glm::mat4& model_matrix,
        float cell_shading_levels
    ) -> void override;
    auto queue_draw_with_color(
        const Renderable& renderable,
        const glm::mat4& model_matrix,
        const glm::vec3& color
    ) -> void override;
    auto draw() -> void override;

private:
    struct DrawCall {
        std::reference_wrapper<const Renderable> renderable;
        glm::mat4 model_matrix;
    };

    struct CellShadingDrawCall {
        std::reference_wrapper<const Renderable> renderable;
        glm::mat4 model_matrix;
        float cell_shading_levels;
    };

    struct ColorDrawCall {
        std::reference_wrapper<const Renderable> renderable;
        glm::mat4 model_matrix;
        glm::vec3 color;
    };

    auto draw_gbuffer_geometry() -> void;
    auto draw_lighting() -> void;
    auto draw_skybox() -> void;
    auto update_lights() -> void;
    auto get_framebuffer_resize_callback() -> Window::FramebufferSizeCallback;

    int32_t opengl_version;

    std::function<int32_t()> get_window_width;
    std::function<int32_t()> get_window_height;

    std::vector<DrawCall> draw_queue;
    std::vector<CellShadingDrawCall> cell_shading_draw_queue;
    std::vector<ColorDrawCall> color_draw_queue;

    OpenGLShader color_shader;
    OpenGLShader phong_shader;
    OpenGLShader skybox_shader;
    OpenGLShader hdr_shader;
    OpenGLShader gbuffer_shader;

    OpenGLFrameBuffer hdr_frame_buffer;
    OpenGLFrameBuffer geometry_frame_buffer;

    OpenGLUniformBuffer<OpenGLUniforms::Transform> transform_uniform_buffer;
    OpenGLUniformBuffer<OpenGLUniforms::Light> light_uniform_buffer;

    OpenGLSkybox skybox;
    OpenGLModel cube;
    OpenGLMesh quad;

    float exposure = 1.0f;

    glm::mat4 view_matrix;
    glm::mat4 projection_matrix;
};

}  // namespace Luminol::Graphics
