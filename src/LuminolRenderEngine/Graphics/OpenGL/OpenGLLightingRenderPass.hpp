#pragma once

#include <gsl/gsl>

#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLDrawCall.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLFrameBuffer.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLShader.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLMesh.hpp>

namespace Luminol::Graphics {

class OpenGLRenderer;

class OpenGLLightingRenderPass {
public:
    OpenGLLightingRenderPass(int32_t width, int32_t height);

    [[nodiscard]] auto get_hdr_frame_buffer() -> OpenGLFrameBuffer&;

    auto draw(OpenGLRenderer& renderer, OpenGLFrameBuffer& gbuffer_frame_buffer)
        -> void;

private:
    OpenGLFrameBuffer hdr_frame_buffer;
    OpenGLShader pbr_shader;
    OpenGLShader hdr_shader;
    OpenGLMesh quad;
};

}  // namespace Luminol::Graphics
