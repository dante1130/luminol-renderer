#include "OpenGLColorRenderPass.hpp"

namespace {

using namespace Luminol::Graphics;

auto create_color_shader() -> OpenGLShader {
    auto color_shader = OpenGLShader{ShaderPaths{
        .vertex_shader_path =
            std::filesystem::path{"res/shaders/color_vert.glsl"},
        .fragment_shader_path =
            std::filesystem::path{"res/shaders/color_frag.glsl"},
        .geometry_shader_path = std::nullopt,
        .tessellation_control_shader_path = std::nullopt,
        .tessellation_evaluation_shader_path = std::nullopt,
        .compute_shader_path = std::nullopt,
    }};

    color_shader.bind();
    color_shader.set_sampler_binding_point(
        "texture_diffuse", SamplerBindingPoint::TextureDiffuse
    );
    color_shader.set_uniform_block_binding_point(
        "Transform", UniformBufferBindingPoint::Transform
    );
    color_shader.set_shader_storage_block_binding_point(
        "InstancingModelMatrix",
        ShaderStorageBufferBindingPoint::InstancingModelMatrices
    );
    color_shader.set_shader_storage_block_binding_point(
        "ColorBuffer", ShaderStorageBufferBindingPoint::Color
    );
    color_shader.unbind();

    return color_shader;
}

}  // namespace

namespace Luminol::Graphics {

OpenGLColorRenderPass::OpenGLColorRenderPass()
    : color_shader{create_color_shader()} {}

auto OpenGLColorRenderPass::draw(
    const RenderableManager& renderable_manager,
    const OpenGLFrameBuffer& hdr_frame_buffer,
    gsl::span<ColorInstancedDrawCall> draw_calls,
    OpenGLShaderStorageBuffer& instancing_model_matrix_buffer,
    OpenGLShaderStorageBuffer& instancing_color_buffer
) const -> void {
    hdr_frame_buffer.bind();
    this->color_shader.bind();

    for (const auto& draw_call : draw_calls) {
        instancing_model_matrix_buffer.set_data(
            0,
            gsl::narrow<int64_t>(
                draw_call.model_matrices.size() *
                sizeof(draw_call.model_matrices[0])
            ),
            draw_call.model_matrices.data()
        );

        instancing_color_buffer.set_data(
            0,
            gsl::narrow<int64_t>(
                draw_call.colors.size() * sizeof(draw_call.colors[0])
            ),
            draw_call.colors.data()
        );

        const auto& renderable =
            renderable_manager.get_renderable(draw_call.renderable_id);

        renderable.draw_instanced(
            gsl::narrow<int32_t>(draw_call.model_matrices.size())
        );
    }

    this->color_shader.unbind();
    hdr_frame_buffer.unbind();
}

}  // namespace Luminol::Graphics
