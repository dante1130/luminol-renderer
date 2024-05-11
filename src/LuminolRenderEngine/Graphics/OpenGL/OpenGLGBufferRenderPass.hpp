#pragma once

#include <gsl/gsl>

#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLDrawCall.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLFrameBuffer.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLShader.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLUniformBuffer.hpp>

namespace Luminol::Graphics {

class OpenGLRenderer;

class OpenGLGBufferRenderPass {
public:
    OpenGLGBufferRenderPass(int32_t width, int32_t height);

    [[nodiscard]] auto get_gbuffer_frame_buffer() -> OpenGLFrameBuffer&;

    auto draw(
        OpenGLRenderer& renderer,
        gsl::span<DrawCall> draw_calls,
        OpenGLUniformBuffer& transform_uniform_buffer
    ) const -> void;

private:
    OpenGLFrameBuffer gbuffer_frame_buffer;
    OpenGLShader gbuffer_shader;
};

}  // namespace Luminol::Graphics
