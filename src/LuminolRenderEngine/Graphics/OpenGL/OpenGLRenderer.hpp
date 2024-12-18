#pragma once

#include <LuminolRenderEngine/Graphics/Renderer.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLUniformBuffer.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLShaderStorageBuffer.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLFrameBuffer.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLGBufferRenderPass.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLLightingRenderPass.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLColorRenderPass.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLPrimitiveRenderPass.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLSkyboxRenderPass.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLHDRRenderPass.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLAutoExposureRenderPass.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLDrawCall.hpp>

namespace Luminol::Graphics {

class OpenGLRenderer : public Renderer {
public:
    OpenGLRenderer(Window& window, GraphicsApi graphics_api);

    auto set_view_matrix(const Maths::Matrix4x4f& view_matrix) -> void override;
    auto set_projection_matrix(const Maths::Matrix4x4f& projection_matrix)
        -> void override;

    auto set_exposure(float exposure) -> void override;

    auto clear_color(const Maths::Vector4f& color) const -> void override;
    auto clear(BufferBit buffer_bit) const -> void override;

    auto queue_draw(
        RenderableId renderable_id, const Maths::Matrix4x4f& model_matrix
    ) -> void override;

    auto queue_draw_with_color(
        RenderableId renderable_id,
        const Maths::Matrix4x4f& model_matrix,
        const Maths::Vector3f& color
    ) -> void override;

    auto queue_draw_line(
        const Maths::Vector3f& start,
        const Maths::Vector3f& end,
        const Maths::Vector3f& color
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
    LineDrawCall line_draw_call;

    std::unordered_map<RenderableId, InstancedDrawCall&>
        instanced_draw_call_map;
    std::unordered_map<RenderableId, ColorInstancedDrawCall&>
        instanced_color_draw_call_map;

    OpenGLFrameBuffer hdr_frame_buffer;

    OpenGLGBufferRenderPass gbuffer_render_pass;
    OpenGLLightingRenderPass lighting_render_pass;
    OpenGLColorRenderPass color_render_pass;
    OpenGLPrimitiveRenderPass primitive_render_pass;
    OpenGLSkyboxRenderPass skybox_render_pass;
    OpenGLHDRRenderPass hdr_render_pass;
    OpenGLAutoExposureRenderPass auto_exposure_render_pass;

    OpenGLUniformBuffer transform_uniform_buffer;
    OpenGLUniformBuffer light_uniform_buffer;
    OpenGLShaderStorageBuffer instancing_model_matrix_buffer;
    OpenGLShaderStorageBuffer instancing_color_buffer;

    float exposure = 1.0f;

    Maths::Matrix4x4f view_matrix;
    Maths::Matrix4x4f projection_matrix;
};

}  // namespace Luminol::Graphics
