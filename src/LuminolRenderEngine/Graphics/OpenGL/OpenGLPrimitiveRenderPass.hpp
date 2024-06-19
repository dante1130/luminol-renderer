#pragma once

#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLDrawCall.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLShader.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLFrameBuffer.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLUniformBuffer.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLShaderStorageBuffer.hpp>

namespace Luminol::Graphics {

class OpenGLPrimitiveRenderPass {
public:
    OpenGLPrimitiveRenderPass();

    auto draw_lines(
        const OpenGLFrameBuffer& hdr_frame_buffer,
        const LineDrawCall& line_draw_call,
        OpenGLShaderStorageBuffer& color_buffer
    ) -> void;

private:
    OpenGLShader line_shader;
};

}  // namespace Luminol::Graphics
