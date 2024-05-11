#pragma once

#include <gsl/gsl>

#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLDrawCall.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLFrameBuffer.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLShader.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLModel.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLShaderStorageBuffer.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLColorRenderPass.hpp>

namespace Luminol::Graphics {

class OpenGLRenderer;

class OpenGLLightingRenderPass {
public:
    OpenGLLightingRenderPass();

    auto draw(
        const OpenGLFrameBuffer& gbuffer_frame_buffer,
        const OpenGLFrameBuffer& hdr_frame_buffer,
        const glm::mat4& view_matrix,
        const OpenGLColorRenderPass& color_render_pass,
        gsl::span<ColorDrawInstancedCall> color_draw_calls,
        OpenGLShaderStorageBuffer& instancing_color_model_matrix_buffer,
        OpenGLShaderStorageBuffer& instancing_color_buffer
    ) const -> void;

private:
    OpenGLShader pbr_shader;
    OpenGLMesh quad;
};

}  // namespace Luminol::Graphics
