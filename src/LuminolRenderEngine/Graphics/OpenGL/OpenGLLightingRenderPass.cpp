#include "OpenGLLightingRenderPass.hpp"

#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLRenderer.hpp>
#include "LuminolRenderEngine/Graphics/OpenGL/OpenGLUniformBindingPoints.hpp"

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

auto create_hdr_frame_buffer(int32_t width, int32_t height)
    -> OpenGLFrameBuffer {
    return OpenGLFrameBuffer{OpenGLFrameBufferDescriptor{
        width,
        height,
        {OpenGLFrameBufferAttachment{
            .internal_format = TextureInternalFormat::RGBA16F,
            .format = TextureFormat::RGBA,
            .binding_point = SamplerBindingPoint::HDRFramebuffer,
        }},
    }};
}

auto create_skybox_shader() -> OpenGLShader {
    auto skybox_shader = OpenGLShader{ShaderPaths{
        .vertex_shader_path =
            std::filesystem::path{"res/shaders/skybox_vert.glsl"},
        .fragment_shader_path =
            std::filesystem::path{"res/shaders/skybox_frag.glsl"},
    }};

    skybox_shader.bind();
    skybox_shader.set_sampler_binding_point(
        "skybox", SamplerBindingPoint::Skybox
    );
    skybox_shader.set_uniform_block_binding_point(
        "Transform", UniformBufferBindingPoint::Transform
    );
    skybox_shader.unbind();

    return skybox_shader;
}

auto get_view_position(const glm::mat4& view_matrix) -> glm::vec3 {
    return glm::inverse(view_matrix)[3];
}

}  // namespace

namespace Luminol::Graphics {

OpenGLLightingRenderPass::OpenGLLightingRenderPass(
    int32_t width, int32_t height
)
    : hdr_frame_buffer{create_hdr_frame_buffer(width, height)},
      pbr_shader{create_pbr_shader()},
      hdr_shader{create_hdr_shader()},
      skybox_shader{create_skybox_shader()},
      cube{"res/models/cube/cube.obj"},
      quad{create_quad_mesh()} {}

auto OpenGLLightingRenderPass::get_hdr_frame_buffer() -> OpenGLFrameBuffer& {
    return hdr_frame_buffer;
}

auto OpenGLLightingRenderPass::draw(
    OpenGLRenderer& renderer,
    OpenGLFrameBuffer& gbuffer_frame_buffer,
    OpenGLSkybox& skybox
) -> void {
    this->hdr_frame_buffer.bind();
    renderer.clear(BufferBit::ColorDepth);

    this->pbr_shader.bind();
    gbuffer_frame_buffer.bind_color_attachments();
    this->pbr_shader.set_uniform(
        "view_position", get_view_position(renderer.get_view_matrix())
    );
    this->quad.draw();
    gbuffer_frame_buffer.unbind_color_attachments();
    this->pbr_shader.unbind();

    gbuffer_frame_buffer.blit_to_framebuffer(
        this->hdr_frame_buffer, BufferBit::Depth
    );

    const auto transform = OpenGLUniforms::Transform{
        .view_matrix = glm::mat4{glm::mat3{renderer.get_view_matrix()}},
        .projection_matrix = renderer.get_projection_matrix(),
    };

    renderer.get_transform_uniform_buffer().set_data(
        0, sizeof(transform), &transform
    );

    glCullFace(GL_FRONT);
    glDepthFunc(GL_LEQUAL);
    this->skybox_shader.bind();
    skybox.bind();
    this->cube.draw();
    skybox.unbind();
    this->skybox_shader.unbind();
    glDepthFunc(GL_LESS);
    glCullFace(GL_BACK);

    this->hdr_frame_buffer.unbind();

    this->hdr_shader.bind();
    this->hdr_shader.set_uniform("exposure", renderer.get_exposure());
    this->hdr_frame_buffer.bind_color_attachments();
    this->quad.draw();
    this->hdr_frame_buffer.unbind_color_attachments();
    this->hdr_shader.unbind();
}

}  // namespace Luminol::Graphics
