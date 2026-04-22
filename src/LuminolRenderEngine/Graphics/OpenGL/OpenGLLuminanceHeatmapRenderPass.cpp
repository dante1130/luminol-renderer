#include "OpenGLLuminanceHeatmapRenderPass.hpp"

#include <glad/gl.h>

namespace {

using namespace Luminol::Graphics;

auto create_heatmap_shader() -> OpenGLShader {
    auto heatmap_shader = OpenGLShader{ShaderPaths{
        .vertex_shader_path =
            std::filesystem::path{"res/shaders/luminance_heatmap_vert.glsl"},
        .fragment_shader_path =
            std::filesystem::path{"res/shaders/luminance_heatmap_frag.glsl"},
        .geometry_shader_path = std::nullopt,
        .tessellation_control_shader_path = std::nullopt,
        .tessellation_evaluation_shader_path = std::nullopt,
        .compute_shader_path = std::nullopt,
    }};

    heatmap_shader.bind();
    heatmap_shader.set_sampler_binding_point(
        "hdr_framebuffer", SamplerBindingPoint::HDRFramebuffer
    );
    heatmap_shader.unbind();

    return heatmap_shader;
}

}  // namespace

namespace Luminol::Graphics {

OpenGLLuminanceHeatmapRenderPass::OpenGLLuminanceHeatmapRenderPass()
    : heatmap_shader{create_heatmap_shader()}, quad{create_quad_mesh()} {}

auto OpenGLLuminanceHeatmapRenderPass::draw(
    const OpenGLFrameBuffer& hdr_frame_buffer
) const -> void {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    hdr_frame_buffer.bind_color_attachments();
    this->heatmap_shader.bind();
    this->quad.draw();
    this->heatmap_shader.unbind();
    hdr_frame_buffer.unbind_color_attachments();

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

}  // namespace Luminol::Graphics
