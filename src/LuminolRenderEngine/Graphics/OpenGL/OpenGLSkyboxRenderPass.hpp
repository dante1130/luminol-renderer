#pragma once

#include <LuminolMaths/Matrix.hpp>

#include <vector>

#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLSkybox.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLShader.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLMesh.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLFrameBuffer.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLUniformBuffer.hpp>

namespace Luminol::Graphics {

class OpenGLSkyboxRenderPass {
public:
    OpenGLSkyboxRenderPass();

    auto draw(
        const OpenGLFrameBuffer& hdr_frame_buffer,
        OpenGLUniformBuffer& transform_uniform_buffer,
        const Maths::Matrix4x4f& view_matrix
    ) const -> void;

private:
    OpenGLSkybox skybox;
    OpenGLShader skybox_shader;
    std::vector<OpenGLMesh> cube;
};

}  // namespace Luminol::Graphics
