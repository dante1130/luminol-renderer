#include "OpenGLAutoExposureRenderPass.hpp"

#include <glad/gl.h>

namespace {

using namespace Luminol::Graphics;

auto create_luminance_histogram_shader() -> OpenGLShader {
    auto luminance_histogram_shader = OpenGLShader{ShaderPaths{
        .vertex_shader_path = std::nullopt,
        .fragment_shader_path = std::nullopt,
        .geometry_shader_path = std::nullopt,
        .tessellation_control_shader_path = std::nullopt,
        .tessellation_evaluation_shader_path = std::nullopt,
        .compute_shader_path = "res/shaders/luminance_histogram.glsl",
    }};

    luminance_histogram_shader.bind();
    luminance_histogram_shader.set_image_binding_point(
        "hdr_image", ImageBindingPoint::HDRFramebuffer
    );
    luminance_histogram_shader.set_shader_storage_block_binding_point(
        "LuminanceHistogramBuffer",
        ShaderStorageBufferBindingPoint::LuminanceHistogram
    );
    luminance_histogram_shader.unbind();

    return luminance_histogram_shader;
}

auto create_average_luminance_shader() -> OpenGLShader {
    auto average_luminance_shader = OpenGLShader{ShaderPaths{
        .vertex_shader_path = std::nullopt,
        .fragment_shader_path = std::nullopt,
        .geometry_shader_path = std::nullopt,
        .tessellation_control_shader_path = std::nullopt,
        .tessellation_evaluation_shader_path = std::nullopt,
        .compute_shader_path = "res/shaders/average_luminance.glsl",
    }};

    average_luminance_shader.bind();
    average_luminance_shader.set_shader_storage_block_binding_point(
        "LuminanceHistogramBuffer",
        ShaderStorageBufferBindingPoint::LuminanceHistogram
    );
    average_luminance_shader.set_image_binding_point(
        "average_luminance_image", ImageBindingPoint::AverageLuminance
    );
    average_luminance_shader.unbind();

    return average_luminance_shader;
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
      average_luminance_shader{create_average_luminance_shader()},
      average_luminance_texture{1, 1, TextureInternalFormat::R16F},
      screen_texture{width, height, TextureInternalFormat::RGBA16F} {}

auto OpenGLAutoExposureRenderPass::draw(
    const OpenGLFrameBuffer& hdr_framebuffer, float delta_time
) -> void {
    constexpr auto histogram_work_group_size = 16;
    constexpr auto initial_histogram_data = std::array<uint32_t, 256>{};

    this->luminance_histogram_shader.bind();
    hdr_framebuffer.bind_image(
        ImageBindingPoint::HDRFramebuffer, ImageAccess::Read
    );
    this->luminance_histogram_buffer.set_data(
        0,
        gsl::narrow<int64_t>(sizeof(initial_histogram_data)),
        initial_histogram_data.data()
    );
    this->luminance_histogram_shader.dispatch_compute(
        hdr_framebuffer.get_width() / histogram_work_group_size,
        hdr_framebuffer.get_height() / histogram_work_group_size,
        1
    );
    this->luminance_histogram_shader.unbind();

    this->average_luminance_shader.bind();
    this->average_luminance_shader.set_uniform("delta_time", delta_time);
    this->average_luminance_shader.set_uniform(
        "num_pixels", hdr_framebuffer.get_width() * hdr_framebuffer.get_height()
    );
    this->average_luminance_texture.bind_image(
        ImageBindingPoint::AverageLuminance, ImageAccess::ReadWrite
    );
    this->average_luminance_shader.dispatch_compute(1, 1, 1);
    this->average_luminance_shader.unbind();
}

}  // namespace Luminol::Graphics
