#include "OpenGLRenderer.hpp"

#include <iostream>

#include <gsl/gsl>
#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>

#include <Engine/Graphics/OpenGL/OpenGLError.hpp>
#include <Engine/Graphics/OpenGL/OpenGLShader.hpp>

namespace {

using namespace Luminol::Graphics;

constexpr auto low_res_frame_buffer_scale = 1;

auto initialize_opengl(
    Luminol::Window& window,
    const Luminol::Window::FramebufferSizeCallback& framebuffer_size_callback
) -> int32_t {
    const auto version = gladLoadGL(window.get_proc_address());
    Ensures(version != 0);

    std::cout << "OpenGL Version " << GLAD_VERSION_MAJOR(version) << "."
              << GLAD_VERSION_MINOR(version) << " loaded\n";

    window.set_framebuffer_size_callback(framebuffer_size_callback);

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(gl_debug_message_callback, nullptr);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CW);

    return version;
}

constexpr auto buffer_bit_to_gl(BufferBit buffer_bit) -> GLenum {
    switch (buffer_bit) {
        case BufferBit::Color:
            return GL_COLOR_BUFFER_BIT;
        case BufferBit::Depth:
            return GL_DEPTH_BUFFER_BIT;
        case BufferBit::Stencil:
            return GL_STENCIL_BUFFER_BIT;
        case BufferBit::ColorDepth:
            return GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
        case BufferBit::ColorStencil:
            return GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
        case BufferBit::DepthStencil:
            return GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
        case BufferBit::ColorDepthStencil:
            return GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
                   GL_STENCIL_BUFFER_BIT;
    }

    return 0;
}

auto create_color_shader() -> OpenGLShader {
    auto color_shader = OpenGLShader{ShaderPaths{
        .vertex_shader_path =
            std::filesystem::path{"res/shaders/color_vert.glsl"},
        .fragment_shader_path =
            std::filesystem::path{"res/shaders/color_frag.glsl"}
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

auto create_phong_shader() -> OpenGLShader {
    auto phong_shader = OpenGLShader{ShaderPaths{
        .vertex_shader_path =
            std::filesystem::path{"res/shaders/phong_vert.glsl"},
        .fragment_shader_path =
            std::filesystem::path{"res/shaders/phong_frag.glsl"}
    }};

    phong_shader.bind();
    phong_shader.set_sampler_binding_point(
        "material.texture_diffuse", SamplerBindingPoint::TextureDiffuse
    );
    phong_shader.set_sampler_binding_point(
        "material.texture_specular", SamplerBindingPoint::TextureSpecular
    );
    phong_shader.set_sampler_binding_point(
        "material.texture_emissive", SamplerBindingPoint::TextureEmissive
    );
    phong_shader.set_sampler_binding_point(
        "material.texture_normal", SamplerBindingPoint::TextureNormal
    );
    phong_shader.set_sampler_binding_point(
        "skybox", SamplerBindingPoint::Skybox
    );
    phong_shader.set_uniform_block_binding_point(
        "Transform", UniformBufferBindingPoint::Transform
    );
    phong_shader.set_uniform_block_binding_point(
        "Light", UniformBufferBindingPoint::Light
    );
    phong_shader.unbind();

    return phong_shader;
}

auto create_skybox_shader() -> OpenGLShader {
    auto skybox_shader = OpenGLShader{ShaderPaths{
        .vertex_shader_path =
            std::filesystem::path{"res/shaders/skybox_vert.glsl"},
        .fragment_shader_path =
            std::filesystem::path{"res/shaders/skybox_frag.glsl"}
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

OpenGLRenderer::OpenGLRenderer(Window& window)
    : opengl_version{initialize_opengl(
          window, this->get_framebuffer_resize_callback()
      )},
      get_window_width{[&window]() { return window.get_width(); }},
      get_window_height{[&window]() { return window.get_height(); }},
      color_shader{create_color_shader()},
      phong_shader{create_phong_shader()},
      skybox_shader{create_skybox_shader()},
      low_res_frame_buffer{
          window.get_width() / low_res_frame_buffer_scale,
          window.get_height() / low_res_frame_buffer_scale
      },
      transform_uniform_buffer{OpenGLUniformBuffer<OpenGLUniforms::Transform>{
          OpenGLUniforms::Transform{}, UniformBufferBindingPoint::Transform
      }},
      light_uniform_buffer{OpenGLUniformBuffer<OpenGLUniforms::Light>{
          OpenGLUniforms::Light{}, UniformBufferBindingPoint::Light
      }},
      skybox{OpenGLSkybox{SkyboxPaths{
          .front = std::filesystem::path{"res/skybox/default/front.jpg"},
          .back = std::filesystem::path{"res/skybox/default/back.jpg"},
          .top = std::filesystem::path{"res/skybox/default/top.jpg"},
          .bottom = std::filesystem::path{"res/skybox/default/bottom.jpg"},
          .left = std::filesystem::path{"res/skybox/default/left.jpg"},
          .right = std::filesystem::path{"res/skybox/default/right.jpg"}
      }}},
      cube{OpenGLModel{"res/models/cube/cube.obj"}},
      view_matrix{glm::mat4{1.0f}},
      projection_matrix{glm::mat4{1.0f}} {}

auto OpenGLRenderer::set_view_matrix(const glm::mat4& view_matrix) -> void {
    this->view_matrix = view_matrix;
}

auto OpenGLRenderer::set_projection_matrix(const glm::mat4& projection_matrix)
    -> void {
    this->projection_matrix = projection_matrix;
}

auto OpenGLRenderer::clear_color(const glm::vec4& color) const -> void {
    glClearColor(color.r, color.g, color.b, color.a);
}

auto OpenGLRenderer::clear(BufferBit buffer_bit) const -> void {
    glClear(buffer_bit_to_gl(buffer_bit));
}

auto OpenGLRenderer::queue_draw_with_phong(
    const RenderCommand& render_command,
    const glm::mat4& model_matrix,
    const Material& material
) -> void {
    this->draw_queue.emplace_back(
        [this, model_matrix, render_command, material] {
            this->transform_uniform_buffer.set_data(OpenGLUniforms::Transform{
                .model_matrix = model_matrix,
                .view_matrix = this->view_matrix,
                .projection_matrix = this->projection_matrix
            });

            this->phong_shader.bind();
            this->phong_shader.set_uniform(
                "view_position", get_view_position(this->view_matrix)
            );
            this->phong_shader.set_uniform(
                "material.shininess", material.shininess
            );
            this->phong_shader.set_uniform("is_cell_shading_enabled", 0);
            render_command();
        }
    );
}

auto OpenGLRenderer::queue_draw_with_cell_shading(
    const RenderCommand& render_command,
    const glm::mat4& model_matrix,
    const Material& material,
    float cell_shading_levels
) -> void {
    Ensures(cell_shading_levels > 0);

    this->draw_queue.emplace_back(
        [this, model_matrix, render_command, material, cell_shading_levels] {
            this->transform_uniform_buffer.set_data(OpenGLUniforms::Transform{
                .model_matrix = model_matrix,
                .view_matrix = this->view_matrix,
                .projection_matrix = this->projection_matrix
            });

            this->phong_shader.bind();
            this->phong_shader.set_uniform(
                "view_position", get_view_position(this->view_matrix)
            );
            this->phong_shader.set_uniform(
                "material.shininess", material.shininess
            );
            this->phong_shader.set_uniform("is_cell_shading_enabled", 1);
            this->phong_shader.set_uniform(
                "cell_shading_levels", cell_shading_levels
            );
            render_command();
        }
    );
}

auto OpenGLRenderer::queue_draw_with_color(
    const RenderCommand& render_command,
    const glm::mat4& model_matrix,
    const glm::vec3& color
) -> void {
    this->draw_queue.emplace_back([this, model_matrix, render_command, color] {
        this->transform_uniform_buffer.set_data(OpenGLUniforms::Transform{
            .model_matrix = model_matrix,
            .view_matrix = this->view_matrix,
            .projection_matrix = this->projection_matrix
        });

        this->color_shader.bind();
        this->color_shader.set_uniform("color", color);
        render_command();
    });
}

auto OpenGLRenderer::draw() -> void {
    const auto draw_scene = [this] {
        for (const auto& draw_call : this->draw_queue) {
            draw_call();
        }
    };

    this->update_lights();

    this->low_res_frame_buffer.bind();
    this->clear(BufferBit::ColorDepth);
    glViewport(
        0,
        0,
        this->low_res_frame_buffer.get_width(),
        this->low_res_frame_buffer.get_height()
    );

    this->draw_skybox();
    draw_scene();

    this->low_res_frame_buffer.unbind();

    this->clear(BufferBit::ColorDepth);
    glViewport(0, 0, this->get_window_width(), this->get_window_height());
    this->low_res_frame_buffer.blit(
        this->get_window_width(), this->get_window_height()
    );

    this->draw_queue.clear();
}

auto OpenGLRenderer::update_lights() -> void {
    const auto& light_data = this->get_light_manager().get_light_data();

    auto light_uniforms = OpenGLUniforms::Light{
        .directional_light =
            {.direction = {light_data.directional_light.direction},
             .ambient = {light_data.directional_light.ambient},
             .diffuse = {light_data.directional_light.diffuse},
             .specular = {light_data.directional_light.specular}},
        .point_light_count = light_data.point_light_count
    };

    /// NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index)
    for (size_t i = 0; i < light_data.point_light_count; ++i) {
        light_uniforms.point_lights[i] = {
            .position = {light_data.point_lights[i].position},
            .ambient = {light_data.point_lights[i].ambient},
            .diffuse = {light_data.point_lights[i].diffuse},
            .specular = {light_data.point_lights[i].specular},
            .constant = light_data.point_lights[i].constant,
            .linear = light_data.point_lights[i].linear,
            .quadratic = light_data.point_lights[i].quadratic,
        };
    }
    /// NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index)

    this->light_uniform_buffer.set_data(light_uniforms);
}

auto OpenGLRenderer::draw_skybox() -> void {
    this->transform_uniform_buffer.set_data(OpenGLUniforms::Transform{
        .view_matrix = glm::mat4{glm::mat3{this->view_matrix}},
        .projection_matrix = this->projection_matrix
    });

    glCullFace(GL_FRONT);
    glDepthFunc(GL_LEQUAL);
    this->skybox_shader.bind();
    this->skybox.bind();
    this->cube.get_render_command()();
    this->skybox.unbind();
    glDepthFunc(GL_LESS);
    glCullFace(GL_BACK);
}

auto OpenGLRenderer::get_framebuffer_resize_callback()
    -> Window::FramebufferSizeCallback {
    return [this](int width, int height) {
        glViewport(0, 0, width, height);
        this->low_res_frame_buffer.resize(
            width / low_res_frame_buffer_scale,
            height / low_res_frame_buffer_scale
        );
    };
}

}  // namespace Luminol::Graphics
