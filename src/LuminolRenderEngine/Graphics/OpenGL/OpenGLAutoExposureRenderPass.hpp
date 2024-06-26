#pragma once

#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLShader.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLTexture.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLMesh.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLShaderStorageBuffer.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLFrameBuffer.hpp>

namespace Luminol::Graphics {

class OpenGLAutoExposureRenderPass {
public:
    OpenGLAutoExposureRenderPass(int32_t width, int32_t height);

    auto draw(const OpenGLFrameBuffer& hdr_framebuffer, float delta_time)
        -> void;

private:
    OpenGLShader luminance_histogram_shader;
    OpenGLShaderStorageBuffer luminance_histogram_buffer;

    OpenGLShader average_luminance_shader;
    OpenGLTexture average_luminance_texture;

    OpenGLTexture screen_texture;
};

}  // namespace Luminol::Graphics
