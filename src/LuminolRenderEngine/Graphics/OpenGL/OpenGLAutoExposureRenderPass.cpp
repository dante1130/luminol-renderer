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

constexpr auto initial_average_luminance = 0.18f;

OpenGLAutoExposureRenderPass::OpenGLAutoExposureRenderPass()
    : luminance_histogram_shader{create_luminance_histogram_shader()},
      luminance_histogram_buffer{
          ShaderStorageBufferBindingPoint::LuminanceHistogram
      },
      average_luminance_shader{create_average_luminance_shader()},
      average_luminance_texture{1, 1, TextureInternalFormat::RGBA16F} {
    this->initialize_average_luminance(initial_average_luminance);
}

auto OpenGLAutoExposureRenderPass::initialize_average_luminance(
    const float value
) -> void {
    const std::array<float, 4> clear_value = {value, 0.0f, 0.0f, 1.0f};
    glClearTexImage(
        this->average_luminance_texture.get_texture_id(),
        0,
        GL_RGBA,
        GL_FLOAT,
        clear_value.data()
    );
}

auto OpenGLAutoExposureRenderPass::draw(
    const OpenGLFrameBuffer& hdr_framebuffer,
    float delta_time,
    const float exposure_multiplier
) -> float {
    constexpr auto histogram_work_group_size = 16;
    constexpr auto initial_histogram_data =
        std::array<uint32_t, histogram_bin_count>{};
    constexpr auto min_luminance = 0.001f;
    constexpr auto middle_gray = 0.18f;

    const auto dispatch_width =
        (hdr_framebuffer.get_width() + histogram_work_group_size - 1) /
        histogram_work_group_size;
    const auto dispatch_height =
        (hdr_framebuffer.get_height() + histogram_work_group_size - 1) /
        histogram_work_group_size;
    const auto total_pixels =
        hdr_framebuffer.get_width() * hdr_framebuffer.get_height();

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
        dispatch_width, dispatch_height, 1
    );

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    this->luminance_histogram_shader.unbind();

    this->average_luminance_shader.bind();
    this->average_luminance_shader.set_uniform("delta_time", delta_time);
    this->average_luminance_shader.set_uniform("num_pixels", total_pixels);
    this->average_luminance_texture.bind_image(
        ImageBindingPoint::AverageLuminance, ImageAccess::ReadWrite
    );
    this->average_luminance_shader.dispatch_compute(1, 1, 1);
    this->average_luminance_shader.unbind();

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    const auto average_luminance_data =
        this->average_luminance_texture.get_data();
    const auto average_luminance = average_luminance_data.empty()
                                       ? middle_gray
                                       : average_luminance_data[0];

    return (middle_gray / std::max(average_luminance, min_luminance)) *
           exposure_multiplier;
}

}  // namespace Luminol::Graphics
