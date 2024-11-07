#include "OpenGLLightingRenderPass.hpp"

namespace {

using namespace Luminol::Graphics;
using namespace Luminol::Maths;

auto create_pbr_shader() -> OpenGLShader {
    auto pbr_shader = OpenGLShader{ShaderPaths{
        .vertex_shader_path =
            std::filesystem::path{"res/shaders/pbr_vert.glsl"},
        .fragment_shader_path =
            std::filesystem::path{"res/shaders/pbr_frag.glsl"},
    }};

    pbr_shader.bind();
    pbr_shader.set_sampler_binding_point(
        "gbuffer.position_metallic",
        SamplerBindingPoint::GBufferPositionMetallic
    );
    pbr_shader.set_sampler_binding_point(
        "gbuffer.normal_roughness", SamplerBindingPoint::GBufferNormalRoughness
    );
    pbr_shader.set_sampler_binding_point(
        "gbuffer.emissive_ao", SamplerBindingPoint::GBufferEmissiveAO
    );
    pbr_shader.set_sampler_binding_point(
        "gbuffer.albedo", SamplerBindingPoint::GBufferAlbedo
    );
    pbr_shader.set_uniform_block_binding_point(
        "Light", UniformBufferBindingPoint::Light
    );
    pbr_shader.unbind();

    return pbr_shader;
}

auto get_view_position(const Matrix4x4f& view_matrix) -> Vector3f {
    const auto inverse_view_matrix = view_matrix.inverse();

    return Vector3f{
        inverse_view_matrix[3][0],
        inverse_view_matrix[3][1],
        inverse_view_matrix[3][2],
    };
}

}  // namespace

namespace Luminol::Graphics {

OpenGLLightingRenderPass::OpenGLLightingRenderPass()
    : pbr_shader{create_pbr_shader()}, quad{create_quad_mesh()} {}

auto OpenGLLightingRenderPass::draw(
    const OpenGLFrameBuffer& gbuffer_frame_buffer,
    const OpenGLFrameBuffer& hdr_frame_buffer,
    const Maths::Matrix4x4f& view_matrix
) const -> void {
    hdr_frame_buffer.bind();

    this->pbr_shader.bind();
    gbuffer_frame_buffer.bind_color_attachments();
    this->pbr_shader.set_uniform(
        "view_position", get_view_position(view_matrix)
    );
    this->quad.draw();
    gbuffer_frame_buffer.unbind_color_attachments();
    this->pbr_shader.unbind();

    gbuffer_frame_buffer.blit_to_framebuffer(
        hdr_frame_buffer, BufferBit::Depth
    );

    hdr_frame_buffer.unbind();
}

}  // namespace Luminol::Graphics
