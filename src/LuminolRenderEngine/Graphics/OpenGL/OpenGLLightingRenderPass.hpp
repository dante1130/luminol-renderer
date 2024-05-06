#pragma once

#include <gsl/gsl>

#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLDrawCall.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLFrameBuffer.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLShader.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLModel.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLSkybox.hpp>

namespace Luminol::Graphics {

class OpenGLRenderer;

class OpenGLLightingRenderPass {
public:
    OpenGLLightingRenderPass(int32_t width, int32_t height);

    [[nodiscard]] auto get_hdr_frame_buffer() -> OpenGLFrameBuffer&;

    auto draw(
        OpenGLRenderer& renderer,
        OpenGLFrameBuffer& gbuffer_frame_buffer,
        OpenGLSkybox& skybox
    ) -> void;

private:
    OpenGLFrameBuffer hdr_frame_buffer;
    OpenGLShader pbr_shader;
    OpenGLShader hdr_shader;
    OpenGLShader skybox_shader;
    OpenGLModel cube;
    OpenGLMesh quad;
};

}  // namespace Luminol::Graphics
