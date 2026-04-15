#pragma once

#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLFrameBuffer.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLShader.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLMesh.hpp>

namespace Luminol::Graphics {

class OpenGLLuminanceHeatmapRenderPass {
public:
    OpenGLLuminanceHeatmapRenderPass();

    auto draw(const OpenGLFrameBuffer& hdr_frame_buffer) const -> void;

private:
    OpenGLShader heatmap_shader;
    OpenGLMesh quad;
};

}  // namespace Luminol::Graphics
