#include "OpenGLHDRRenderPass.hpp"

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

auto create_hdr_shader() -> OpenGLShader {
    auto hdr_shader = OpenGLShader{ShaderPaths{
        .vertex_shader_path =
            std::filesystem::path{"res/shaders/hdr_vert.glsl"},
        .fragment_shader_path =
            std::filesystem::path{"res/shaders/hdr_frag.glsl"},
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
