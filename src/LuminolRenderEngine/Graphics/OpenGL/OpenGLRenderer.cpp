#include "OpenGLRenderer.hpp"

#include <iostream>

#include <gsl/gsl>
#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>

#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLError.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLShader.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLBufferBit.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLUniformBindingPoints.hpp>

namespace {

using namespace Luminol::Graphics;

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

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CW);

    return version;
}

auto create_transform_uniform_buffer() -> OpenGLUniformBuffer {
    return OpenGLUniformBuffer{
        UniformBufferBindingPoint::Transform, sizeof(OpenGLUniforms::Transform)
    };
}

auto create_light_uniform_buffer() -> OpenGLUniformBuffer {
    const auto size_bytes =
        sizeof(OpenGLUniforms::Light::directional_light) +
        sizeof(OpenGLUniforms::Light::point_light_count) +
        sizeof(OpenGLUniforms::Light::spot_light_count) +
        sizeof(OpenGLUniforms::Light::padding) +
        sizeof(OpenGLUniforms::PointLight) * max_point_lights +
        sizeof(OpenGLUniforms::SpotLight) * max_spot_lights;

    return OpenGLUniformBuffer{
        UniformBufferBindingPoint::Light, gsl::narrow<int64_t>(size_bytes)
    };
}

}  // namespace

namespace Luminol::Graphics {

OpenGLRenderer::OpenGLRenderer(Window& window)
    : opengl_version{initialize_opengl(
          window, this->get_framebuffer_resize_callback()
      )},
      get_window_width{[&window]() { return window.get_width(); }},
      get_window_height{[&window]() { return window.get_height(); }},
      gbuffer_render_pass{this->get_window_width(), this->get_window_height()},
      lighting_render_pass{this->get_window_width(), this->get_window_height()},
      transform_uniform_buffer{create_transform_uniform_buffer()},
      light_uniform_buffer{create_light_uniform_buffer()},
      instancing_model_matrix_buffer{
          ShaderStorageBufferBindingPoint::InstancingModelMatrices
      },
      instancing_color_buffer{ShaderStorageBufferBindingPoint::Color},
      skybox{SkyboxPaths{
          .front = std::filesystem::path{"res/skybox/default/front.jpg"},
          .back = std::filesystem::path{"res/skybox/default/back.jpg"},
          .top = std::filesystem::path{"res/skybox/default/top.jpg"},
          .bottom = std::filesystem::path{"res/skybox/default/bottom.jpg"},
          .left = std::filesystem::path{"res/skybox/default/left.jpg"},
          .right = std::filesystem::path{"res/skybox/default/right.jpg"},
      }},
      view_matrix{glm::mat4{1.0f}},
      projection_matrix{glm::mat4{1.0f}} {}

auto OpenGLRenderer::set_view_matrix(const glm::mat4& view_matrix) -> void {
    this->view_matrix = view_matrix;
}

auto OpenGLRenderer::set_projection_matrix(const glm::mat4& projection_matrix)
    -> void {
    this->projection_matrix = projection_matrix;
}

auto OpenGLRenderer::set_exposure(float exposure) -> void {
    this->exposure = exposure;
}

auto OpenGLRenderer::clear_color(const glm::vec4& color) const -> void {
    glClearColor(color.r, color.g, color.b, color.a);
}

auto OpenGLRenderer::clear(BufferBit buffer_bit) const -> void {
    glClear(buffer_bit_to_gl(buffer_bit));
}

auto OpenGLRenderer::queue_draw(
    const Renderable& renderable, const glm::mat4& model_matrix
) -> void {
    this->draw_queue.emplace_back(renderable, model_matrix);
}

auto OpenGLRenderer::queue_draw_with_color(
    const Renderable& renderable,
    const glm::mat4& model_matrix,
    const glm::vec3& color
) -> void {
    this->color_draw_queue.emplace_back(renderable, model_matrix, color);
}

auto OpenGLRenderer::queue_draw_with_color_instanced(
    const Renderable& renderable,
    gsl::span<glm::mat4> model_matrices,
    gsl::span<glm::vec3> colors
) -> void {
    this->instanced_color_draw_queue.emplace_back(
        renderable, model_matrices, colors
    );
}

auto OpenGLRenderer::draw() -> void {
    this->update_lights();

    auto transform = OpenGLUniforms::Transform{
        .view_matrix = this->view_matrix,
        .projection_matrix = this->projection_matrix,
    };

    this->transform_uniform_buffer.set_data(
        0, sizeof(OpenGLUniforms::Transform), &transform
    );

    this->gbuffer_render_pass.draw(
        *this, this->draw_queue, this->transform_uniform_buffer
    );

    this->lighting_render_pass.draw(
        *this,
        this->gbuffer_render_pass.get_gbuffer_frame_buffer(),
        this->transform_uniform_buffer,
        this->skybox,
        this->view_matrix,
        this->exposure
    );

    this->gbuffer_render_pass.get_gbuffer_frame_buffer()
        .blit_to_default_framebuffer(
            this->get_window_width(),
            this->get_window_height(),
            BufferBit::Depth
        );

    this->transform_uniform_buffer.set_data(
        0, sizeof(OpenGLUniforms::Transform), &transform
    );

    this->color_render_pass.draw(
        this->instanced_color_draw_queue,
        this->instancing_model_matrix_buffer,
        this->instancing_color_buffer
    );

    this->draw_queue.clear();
    this->color_draw_queue.clear();
    this->instanced_color_draw_queue.clear();
}

auto OpenGLRenderer::update_lights() -> void {
    const auto& light_data = this->get_light_manager().get_light_data();

    auto light_uniforms = OpenGLUniforms::Light{
        .directional_light =
            {.direction = {light_data.directional_light.direction},
             .color = {light_data.directional_light.color}},
        .point_light_count = light_data.point_light_count,
        .spot_light_count = light_data.spot_light_count,
    };

    light_uniforms.point_lights.resize(light_data.point_light_count);
    light_uniforms.spot_lights.resize(light_data.spot_light_count);

    /// NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index)
    for (size_t i = 0; i < light_data.point_light_count; ++i) {
        light_uniforms.point_lights[i] = {
            .position = {light_data.point_lights[i].position},
            .color = {light_data.point_lights[i].color},
        };
    }

    for (size_t i = 0; i < light_data.spot_light_count; ++i) {
        light_uniforms.spot_lights[i] = {
            .position = {light_data.spot_lights[i].position},
            .direction = {light_data.spot_lights[i].direction},
            .color = {light_data.spot_lights[i].color},
            .cut_off = light_data.spot_lights[i].cut_off,
            .outer_cut_off = light_data.spot_lights[i].outer_cut_off,
        };
    }
    /// NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index)

    auto offset = int64_t{0};

    this->light_uniform_buffer.set_data(
        offset,
        sizeof(light_uniforms.directional_light),
        &light_uniforms.directional_light
    );

    offset += sizeof(light_uniforms.directional_light);

    this->light_uniform_buffer.set_data(
        offset,
        sizeof(light_uniforms.point_light_count),
        &light_uniforms.point_light_count
    );

    offset += sizeof(light_uniforms.point_light_count);

    this->light_uniform_buffer.set_data(
        offset,
        sizeof(light_uniforms.spot_light_count),
        &light_uniforms.spot_light_count
    );

    offset += sizeof(light_uniforms.spot_light_count) +
              sizeof(light_uniforms.padding);

    this->light_uniform_buffer.set_data(
        offset,
        gsl::narrow<int64_t>(
            sizeof(light_uniforms.point_lights[0]) *
            light_uniforms.point_lights.size()
        ),
        light_uniforms.point_lights.data()
    );

    offset += gsl::narrow<int64_t>(
        sizeof(light_uniforms.point_lights[0]) * max_point_lights
    );

    this->light_uniform_buffer.set_data(
        offset,
        gsl::narrow<int64_t>(
            sizeof(light_uniforms.spot_lights[0]) *
            light_uniforms.spot_lights.size()
        ),
        light_uniforms.spot_lights.data()
    );
}

auto OpenGLRenderer::get_framebuffer_resize_callback()
    -> Window::FramebufferSizeCallback {
    return [this](int width, int height) {
        glViewport(0, 0, width, height);
        this->gbuffer_render_pass.get_gbuffer_frame_buffer().resize(
            width, height
        );
        this->lighting_render_pass.get_hdr_frame_buffer().resize(width, height);
    };
}

}  // namespace Luminol::Graphics
