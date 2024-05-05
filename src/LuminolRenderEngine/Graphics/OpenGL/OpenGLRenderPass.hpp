#pragma once

#include <gsl/gsl>

#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLDrawCall.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLFrameBuffer.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLShader.hpp>

namespace Luminol::Graphics {

class OpenGLRenderer;

class OpenGLGBufferRenderPass{
public:
    OpenGLGBufferRenderPass();
    auto draw(const OpenGLRenderer& renderer, gsl::span<DrawCall> draw_call) -> void;

private:
    OpenGLFrameBuffer gbuffer_frame_buffer;
    OpenGLShader gbuffer_shader;
};

}  // namespace Luminol::Graphics
