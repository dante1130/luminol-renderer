#include "OpenGLHDRRenderPass.hpp"

namespace {

using namespace Luminol::Graphics;

auto create_hdr_shader() -> OpenGLShader {
    auto hdr_shader = OpenGLShader{ShaderPaths{
        .vertex_shader_path =
            std::filesystem::path{"res/shaders/hdr_vert.glsl"},
        .fragment_shader_path =
            std::filesystem::path{"res/shaders/hdr_frag.glsl"},
        .geometry_shader_path = std::nullopt,
        .tessellation_control_shader_path = std::nullopt,
        .tessellation_evaluation_shader_path = std::nullopt,
        .compute_shader_path = std::nullopt,
    }};

    hdr_shader.bind();
    hdr_shader.set_sampler_binding_point(
        "hdr_framebuffer", SamplerBindingPoint::HDRFramebuffer
    );
    hdr_shader.unbind();

    return hdr_shader;
}

}  // namespace

namespace Luminol::Graphics {

OpenGLHDRRenderPass::OpenGLHDRRenderPass()
    : hdr_shader{create_hdr_shader()}, quad{create_quad_mesh()} {}

auto OpenGLHDRRenderPass::draw(
    const OpenGLFrameBuffer& hdr_frame_buffer, float exposure
) const -> void {
    this->hdr_shader.bind();
    this->hdr_shader.set_uniform("exposure", exposure);
    hdr_frame_buffer.bind_color_attachments();
    this->quad.draw();
    hdr_frame_buffer.unbind_color_attachments();
    this->hdr_shader.unbind();
}

}  // namespace Luminol::Graphics
