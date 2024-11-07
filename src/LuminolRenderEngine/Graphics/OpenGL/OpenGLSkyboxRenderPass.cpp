#include "OpenGLSkyboxRenderPass.hpp"

#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLUniforms.hpp>

namespace {

using namespace Luminol::Graphics;

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

}  // namespace

namespace Luminol::Graphics {

OpenGLSkyboxRenderPass::OpenGLSkyboxRenderPass()
    : skybox{SkyboxPaths{
          .front = std::filesystem::path{"res/skybox/default/front.jpg"},
          .back = std::filesystem::path{"res/skybox/default/back.jpg"},
          .top = std::filesystem::path{"res/skybox/default/top.jpg"},
          .bottom = std::filesystem::path{"res/skybox/default/bottom.jpg"},
          .left = std::filesystem::path{"res/skybox/default/left.jpg"},
          .right = std::filesystem::path{"res/skybox/default/right.jpg"},
      }},
      skybox_shader{create_skybox_shader()},
      cube{"res/models/cube/cube.obj"} {}

auto OpenGLSkyboxRenderPass::draw(
    const OpenGLFrameBuffer& hdr_frame_buffer,
    OpenGLUniformBuffer& transform_uniform_buffer,
    const Maths::Matrix4x4f& view_matrix
) const -> void {
    hdr_frame_buffer.bind();

    const auto skybox_view_matrix = Maths::Matrix4x4f{std::array{
        std::array{
            view_matrix[0][0], view_matrix[0][1], view_matrix[0][2], 0.0f
        },
        std::array{
            view_matrix[1][0], view_matrix[1][1], view_matrix[1][2], 0.0f
        },
        std::array{
            view_matrix[2][0], view_matrix[2][1], view_matrix[2][2], 0.0f
        },
        std::array{0.0f, 0.0f, 0.0f, 1.0f},
    }};

    transform_uniform_buffer.set_data(
        offsetof(OpenGLUniforms::Transform, view_matrix),
        sizeof(skybox_view_matrix),
        &skybox_view_matrix
    );

    glCullFace(GL_FRONT);
    glDepthFunc(GL_LEQUAL);
    this->skybox_shader.bind();
    this->skybox.bind();
    this->cube.draw();
    this->skybox.unbind();
    this->skybox_shader.unbind();
    glDepthFunc(GL_LESS);
    glCullFace(GL_BACK);

    hdr_frame_buffer.unbind();
}

}  // namespace Luminol::Graphics
