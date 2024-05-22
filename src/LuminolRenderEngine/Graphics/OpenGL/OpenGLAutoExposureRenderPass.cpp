#include "OpenGLAutoExposureRenderPass.hpp"

#include <glad/gl.h>

namespace {

using namespace Luminol::Graphics;

auto create_luminance_histogram_shader() -> OpenGLShader {
    auto luminance_histogram_shader = OpenGLShader{ShaderPaths{
        .compute_shader_path = "res/shaders/luminance_histogram.glsl",
    }};

    luminance_histogram_shader.bind();
    luminance_histogram_shader.set_image_binding_point(
        "hdr_image", ImageBindingPoint::HDRFramebuffer
    );
    luminance_histogram_shader.unbind();

    return luminance_histogram_shader;
}

}  // namespace

namespace Luminol::Graphics {

OpenGLAutoExposureRenderPass::OpenGLAutoExposureRenderPass(
    int32_t width, int32_t height
)
    : luminance_histogram_shader{create_luminance_histogram_shader()},
      luminance_histogram_buffer{
          ShaderStorageBufferBindingPoint::LuminanceHistogram
      },
      screen_texture{width, height, TextureInternalFormat::RGBA32F} {}

auto OpenGLAutoExposureRenderPass::draw(const OpenGLFrameBuffer& hdr_framebuffer
) const -> void {
    constexpr auto work_group_size = 16;

    this->luminance_histogram_shader.bind();
    hdr_framebuffer.bind_image(
        ImageBindingPoint::HDRFramebuffer, ImageAccess::ReadWrite
    );
    this->luminance_histogram_shader.dispatch_compute(
        hdr_framebuffer.get_width() / work_group_size,
        hdr_framebuffer.get_height() / work_group_size,
        1
    );
    this->luminance_histogram_shader.unbind();
}

}  // namespace Luminol::Graphics
