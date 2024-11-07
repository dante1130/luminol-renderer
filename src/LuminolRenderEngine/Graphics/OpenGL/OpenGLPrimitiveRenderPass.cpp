#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLPrimitiveRenderPass.hpp>

#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLVertexBuffer.hpp>

namespace {

using namespace Luminol::Graphics;

auto create_line_shader() -> OpenGLShader {
    auto line_shader = OpenGLShader{ShaderPaths{
        .vertex_shader_path =
            std::filesystem::path{"res/shaders/line_vert.glsl"},
        .fragment_shader_path =
            std::filesystem::path{"res/shaders/line_frag.glsl"},
        .geometry_shader_path = std::nullopt,
        .tessellation_control_shader_path = std::nullopt,
        .tessellation_evaluation_shader_path = std::nullopt,
        .compute_shader_path = std::nullopt,
    }};

    line_shader.bind();
    line_shader.set_uniform_block_binding_point(
        "Transform", UniformBufferBindingPoint::Transform
    );
    line_shader.set_shader_storage_block_binding_point(
        "ColorBuffer", ShaderStorageBufferBindingPoint::Color
    );
    line_shader.unbind();

    return line_shader;
}

}  // namespace

namespace Luminol::Graphics {

OpenGLPrimitiveRenderPass::OpenGLPrimitiveRenderPass()
    : line_shader{create_line_shader()} {}

auto OpenGLPrimitiveRenderPass::draw_lines(
    const OpenGLFrameBuffer& hdr_frame_buffer,
    const LineDrawCall& line_draw_call,
    OpenGLShaderStorageBuffer& color_buffer
) -> void {
    hdr_frame_buffer.bind();
    this->line_shader.bind();

    color_buffer.set_data(
        0,
        gsl::narrow<int64_t>(
            line_draw_call.colors.size() * sizeof(line_draw_call.colors[0])
        ),
        line_draw_call.colors.data()
    );

    constexpr static auto line_position_component_count = 6;

    const auto line_vertex_buffer = OpenGLVertexBuffer{gsl::span<const float>(
        std::bit_cast<const float*>(line_draw_call.lines.data()),
        line_draw_call.lines.size() * line_position_component_count
    )};

    line_vertex_buffer.bind();

    constexpr static auto line_position_count = 2;

    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        (line_position_component_count / line_position_count) * sizeof(float),
        nullptr
    );
    glEnableVertexAttribArray(0);

    glDrawArrays(
        GL_LINES,
        0,
        gsl::narrow<GLsizei>(line_draw_call.lines.size() * line_position_count)
    );

    this->line_shader.unbind();
    hdr_frame_buffer.unbind();
}

}  // namespace Luminol::Graphics
