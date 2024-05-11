#pragma once

#include <unordered_set>

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
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLSkyboxRenderPass.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLHDRRenderPass.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLDrawCall.hpp>

namespace Luminol::Graphics {

class OpenGLRenderer : public Renderer {
public:
    OpenGLRenderer(Window& window, GraphicsApi graphics_api);

    auto set_view_matrix(const glm::mat4& view_matrix) -> void override;
    auto set_projection_matrix(const glm::mat4& projection_matrix)
        -> void override;

    auto set_exposure(float exposure) -> void override;

    auto clear_color(const glm::vec4& color) const -> void override;
    auto clear(BufferBit buffer_bit) const -> void override;

    auto queue_draw(RenderableId renderable_id, const glm::mat4& model_matrix)
        -> void override;

    auto queue_draw_with_color(
        RenderableId renderable_id,
        const glm::mat4& model_matrix,
        const glm::vec3& color
    ) -> void override;

    auto draw() -> void override;

private:
    auto update_lights() -> void;
    auto get_framebuffer_resize_callback() -> Window::FramebufferSizeCallback;

    int32_t opengl_version;

    std::function<int32_t()> get_window_width;
    std::function<int32_t()> get_window_height;

    std::vector<InstancedDrawCall> instanced_draw_queue;
    std::vector<ColorInstancedDrawCall> instanced_color_draw_queue;

    std::unordered_map<RenderableId, InstancedDrawCall&>
        instanced_draw_call_map;
    std::unordered_map<RenderableId, ColorInstancedDrawCall&>
        instanced_color_draw_call_map;

    OpenGLFrameBuffer hdr_frame_buffer;

    OpenGLGBufferRenderPass gbuffer_render_pass;
    OpenGLLightingRenderPass lighting_render_pass;
    OpenGLColorRenderPass color_render_pass;
    OpenGLSkyboxRenderPass skybox_render_pass;
    OpenGLHDRRenderPass hdr_render_pass;

    OpenGLUniformBuffer transform_uniform_buffer;
    OpenGLUniformBuffer light_uniform_buffer;
    OpenGLShaderStorageBuffer instancing_model_matrix_buffer;
    OpenGLShaderStorageBuffer instancing_color_buffer;

    float exposure = 1.0f;

    glm::mat4 view_matrix;
    glm::mat4 projection_matrix;
};

}  // namespace Luminol::Graphics
