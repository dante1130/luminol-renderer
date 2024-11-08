#pragma once

#include <gsl/gsl>

#include <LuminolMaths/Matrix.hpp>

#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLDrawCall.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLFrameBuffer.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLShader.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLMesh.hpp>

namespace Luminol::Graphics {

class OpenGLRenderer;

class OpenGLLightingRenderPass {
public:
    OpenGLLightingRenderPass();

    auto draw(
        const OpenGLFrameBuffer& gbuffer_frame_buffer,
        const OpenGLFrameBuffer& hdr_frame_buffer,
        const Maths::Matrix4x4f& view_matrix
    ) const -> void;

private:
    OpenGLShader pbr_shader;
    OpenGLMesh quad;
};

}  // namespace Luminol::Graphics
