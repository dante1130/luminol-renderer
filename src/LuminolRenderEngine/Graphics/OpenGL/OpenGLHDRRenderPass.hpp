#pragma once

#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLFrameBuffer.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLShader.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLMesh.hpp>

namespace Luminol::Graphics {

class OpenGLHDRRenderPass {
public:
    OpenGLHDRRenderPass();

    auto draw(const OpenGLFrameBuffer& hdr_frame_buffer, float exposure) const
        -> void;

private:
    OpenGLShader hdr_shader;
    OpenGLMesh quad;
};

}  // namespace Luminol::Graphics
