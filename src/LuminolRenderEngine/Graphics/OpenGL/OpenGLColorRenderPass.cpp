#include "OpenGLColorRenderPass.hpp"

#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLRenderer.hpp>

namespace {

using namespace Luminol::Graphics;

auto create_color_shader() -> OpenGLShader {
    auto color_shader = OpenGLShader{ShaderPaths{
        .vertex_shader_path =
            std::filesystem::path{"res/shaders/color_vert.glsl"},
        .fragment_shader_path =
            std::filesystem::path{"res/shaders/color_frag.glsl"},
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
    OpenGLRenderer& renderer, gsl::span<ColorDrawInstancedCall> draw_calls
) -> void {
    this->color_shader.bind();
    renderer.get_transform_uniform_buffer().set_data(OpenGLUniforms::Transform{
        .view_matrix = renderer.get_view_matrix(),
        .projection_matrix = renderer.get_projection_matrix(),
    });

    for (const auto& draw_call : draw_calls) {
        renderer.get_instancing_model_matrix_buffer().set_data(
            0,
            gsl::narrow<int64_t>(draw_call.model_matrices.size_bytes()),
            draw_call.model_matrices.data()
        );

        auto color_buffer_padded = std::vector<glm::vec4>{};
        color_buffer_padded.reserve(draw_call.colors.size());

        for (const auto& color : draw_call.colors) {
            color_buffer_padded.emplace_back(color, 0.0f);
        }

        renderer.get_instancing_color_buffer().set_data(
            0,
            gsl::narrow<int64_t>(
                color_buffer_padded.size() * sizeof(glm::vec4)
            ),
            color_buffer_padded.data()
        );

        draw_call.renderable.get().draw_instanced(
            gsl::narrow<int32_t>(draw_call.model_matrices.size())
        );
    }

    this->color_shader.unbind();
}

}  // namespace Luminol::Graphics
