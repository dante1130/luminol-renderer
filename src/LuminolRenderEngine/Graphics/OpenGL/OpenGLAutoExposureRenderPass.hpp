#pragma once

#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLShader.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLTexture.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLMesh.hpp>

namespace Luminol::Graphics {

class OpenGLAutoExposureRenderPass {
public:
    OpenGLAutoExposureRenderPass(int32_t width, int32_t height);

    auto draw(int32_t width, int32_t height) const -> void;

private:
    OpenGLShader luminance_histogram_shader;
    OpenGLTexture screen_texture;
};

}  // namespace Luminol::Graphics
