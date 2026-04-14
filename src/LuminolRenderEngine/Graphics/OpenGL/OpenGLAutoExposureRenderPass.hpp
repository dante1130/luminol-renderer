#pragma once

#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLShader.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLTexture.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLMesh.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLShaderStorageBuffer.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLFrameBuffer.hpp>

namespace Luminol::Graphics {

constexpr auto histogram_bin_count = 256;

class OpenGLAutoExposureRenderPass {
public:
    OpenGLAutoExposureRenderPass(int32_t width, int32_t height);

    auto initialize_average_luminance(float value) -> void;

    [[nodiscard]] auto draw(
        const OpenGLFrameBuffer& hdr_framebuffer,
        float delta_time,
        float exposure_multiplier
    ) -> float;

private:
    OpenGLShader luminance_histogram_shader;
    OpenGLShaderStorageBuffer luminance_histogram_buffer;

    OpenGLShader average_luminance_shader;
    OpenGLTexture average_luminance_texture;
};

}  // namespace Luminol::Graphics
