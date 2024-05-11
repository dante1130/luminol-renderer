#include "OpenGLLightingRenderPass.hpp"

namespace {

using namespace Luminol::Graphics;

auto create_quad_mesh() -> OpenGLMesh {
    struct Vertex {
        glm::vec3 position;
        glm::vec2 tex_coords;
        glm::vec3 normal;
        glm::vec3 tangent;
    };

    constexpr auto vertices = std::array{
        Vertex{
            .position = glm::vec3{-1.0f, 1.0f, 0.0f},
            .tex_coords = glm::vec2{0.0f, 1.0f},
            .normal = glm::vec3{0.0f, 0.0f, 1.0f},
            .tangent = glm::vec3{1.0f, 0.0f, 0.0f},
        },
        Vertex{
            .position = glm::vec3{-1.0f, -1.0f, 0.0f},
            .tex_coords = glm::vec2{0.0f, 0.0f},
            .normal = glm::vec3{0.0f, 0.0f, 1.0f},
            .tangent = glm::vec3{1.0f, 0.0f, 0.0f},
        },
        Vertex{
            .position = glm::vec3{1.0f, -1.0f, 0.0f},
            .tex_coords = glm::vec2{1.0f, 0.0f},
            .normal = glm::vec3{0.0f, 0.0f, 1.0f},
            .tangent = glm::vec3{1.0f, 0.0f, 0.0f},
        },
        Vertex{
            .position = glm::vec3{1.0f, 1.0f, 0.0f},
            .tex_coords = glm::vec2{1.0f, 1.0f},
            .normal = glm::vec3{0.0f, 0.0f, 1.0f},
            .tangent = glm::vec3{1.0f, 0.0f, 0.0f},
        }
    };

    // Draw in counter-clockwise order
    constexpr auto indices = std::array{0u, 3u, 2u, 2u, 1u, 0u};

    constexpr auto component_count = 11u;

    auto vertices_float = std::vector<float>{};
    vertices_float.reserve(vertices.size() * component_count);

    for (const auto& vertex : vertices) {
        vertices_float.push_back(vertex.position.x);
        vertices_float.push_back(vertex.position.y);
        vertices_float.push_back(vertex.position.z);

        vertices_float.push_back(vertex.tex_coords.x);
        vertices_float.push_back(vertex.tex_coords.y);

        vertices_float.push_back(vertex.normal.x);
        vertices_float.push_back(vertex.normal.y);
        vertices_float.push_back(vertex.normal.z);

        vertices_float.push_back(vertex.tangent.x);
        vertices_float.push_back(vertex.tangent.y);
        vertices_float.push_back(vertex.tangent.z);
    }

    return OpenGLMesh{vertices_float, indices};
}

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

auto get_view_position(const glm::mat4& view_matrix) -> glm::vec3 {
    return glm::inverse(view_matrix)[3];
}

}  // namespace

namespace Luminol::Graphics {

OpenGLLightingRenderPass::OpenGLLightingRenderPass()
    : pbr_shader{create_pbr_shader()}, quad{create_quad_mesh()} {}

auto OpenGLLightingRenderPass::draw(
    const OpenGLFrameBuffer& gbuffer_frame_buffer,
    const OpenGLFrameBuffer& hdr_frame_buffer,
    const glm::mat4& view_matrix,
    const OpenGLColorRenderPass& color_render_pass,
    gsl::span<ColorDrawInstancedCall> color_draw_calls,
    OpenGLShaderStorageBuffer& instancing_color_model_matrix_buffer,
    OpenGLShaderStorageBuffer& instancing_color_buffer
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

    color_render_pass.draw(
        color_draw_calls,
        instancing_color_model_matrix_buffer,
        instancing_color_buffer
    );

    hdr_frame_buffer.unbind();
}

}  // namespace Luminol::Graphics
