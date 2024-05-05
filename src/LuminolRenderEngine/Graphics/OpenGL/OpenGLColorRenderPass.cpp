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
    color_shader.unbind();

    return color_shader;
}

}  // namespace

namespace Luminol::Graphics {

OpenGLColorRenderPass::OpenGLColorRenderPass()
    : color_shader{create_color_shader()} {}

auto OpenGLColorRenderPass::draw(
    OpenGLRenderer& renderer, gsl::span<ColorDrawCall> draw_calls
) -> void {
    this->color_shader.bind();
    this->color_shader.set_uniform(
        "view_position", glm::inverse(renderer.get_view_matrix())[3]
    );

    for (const auto& draw_call : draw_calls) {
        renderer.get_transform_uniform_buffer().set_data(
            OpenGLUniforms::Transform{
                .model_matrix = draw_call.model_matrix,
                .view_matrix = renderer.get_view_matrix(),
                .projection_matrix = renderer.get_projection_matrix(),
            }
        );
        this->color_shader.set_uniform("color", draw_call.color);

        draw_call.renderable.get().draw();
    }

    this->color_shader.unbind();
}

}  // namespace Luminol::Graphics
