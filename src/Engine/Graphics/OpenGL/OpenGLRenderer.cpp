#include "OpenGLRenderer.hpp"

#include <iostream>
#include <filesystem>

#include <gsl/gsl>
#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>

#include <Engine/Graphics/OpenGL/OpenGLError.hpp>
#include <Engine/Graphics/OpenGL/OpenGLShader.hpp>

namespace {

using namespace Luminol::Graphics;

constexpr auto low_res_frame_buffer_scale = 1;

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

auto create_color_shader() -> std::unique_ptr<OpenGLShader> {
    return std::make_unique<OpenGLShader>(ShaderPaths{
        .vertex_shader_path =
            std::filesystem::path{"res/Shaders/color_vert.glsl"},
        .fragment_shader_path =
            std::filesystem::path{"res/Shaders/color_frag.glsl"}
    });
}

auto create_phong_shader() -> std::unique_ptr<OpenGLShader> {
    auto phong_shader = std::make_unique<OpenGLShader>(ShaderPaths{
        .vertex_shader_path =
            std::filesystem::path{"res/Shaders/phong_vert.glsl"},
        .fragment_shader_path =
            std::filesystem::path{"res/Shaders/phong_frag.glsl"}
    });

    phong_shader->bind();
    phong_shader->set_sampler_binding_point(
        "material.texture_diffuse", SamplerBindingPoint::TextureDiffuse
    );
    phong_shader->set_sampler_binding_point(
        "material.texture_specular", SamplerBindingPoint::TextureSpecular
    );
    phong_shader->set_sampler_binding_point(
        "material.texture_emissive", SamplerBindingPoint::TextureEmissive
    );
    phong_shader->set_sampler_binding_point(
        "material.texture_normal", SamplerBindingPoint::TextureNormal
    );
    phong_shader->set_uniform_block_binding_point(
        "Transform", UniformBufferBindingPoint::Transform
    );
    phong_shader->set_uniform_block_binding_point(
        "Light", UniformBufferBindingPoint::Light
    );
    phong_shader->unbind();

    return phong_shader;
}

auto get_view_position(const glm::mat4& view_matrix) -> glm::vec3 {
    return glm::inverse(view_matrix)[3];
}

}  // namespace

namespace Luminol::Graphics {

OpenGLRenderer::OpenGLRenderer(Window& window)
    : get_window_width{[&window]() { return window.get_width(); }},
      get_window_height{[&window]() { return window.get_height(); }} {
    const auto version = gladLoadGL(window.get_proc_address());
    Ensures(version != 0);

    std::cout << "OpenGL Version " << GLAD_VERSION_MAJOR(version) << "."
              << GLAD_VERSION_MINOR(version) << " loaded\n";

    window.set_framebuffer_size_callback([this](int32_t width, int32_t height) {
        glViewport(0, 0, width, height);
        this->low_res_frame_buffer->resize(
            width / low_res_frame_buffer_scale,
            height / low_res_frame_buffer_scale
        );
    });

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(gl_debug_message_callback, nullptr);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CW);

    this->color_shader = create_color_shader();
    this->phong_shader = create_phong_shader();

    this->low_res_frame_buffer = std::make_unique<OpenGLFrameBuffer>(
        window.get_width() / low_res_frame_buffer_scale,
        window.get_height() / low_res_frame_buffer_scale
    );

    this->transform_uniform_buffer =
        std::make_unique<OpenGLUniformBuffer<OpenGLUniforms::Transform>>(
            OpenGLUniforms::Transform{}, UniformBufferBindingPoint::Transform
        );

    this->light_uniform_buffer =
        std::make_unique<OpenGLUniformBuffer<OpenGLUniforms::Light>>(
            OpenGLUniforms::Light{}, UniformBufferBindingPoint::Light
        );
}

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

auto OpenGLRenderer::update_light(const Light& light) -> void {
    this->light_uniform_buffer->set_data(OpenGLUniforms::Light{
        .position = {light.position},
        .ambient = {light.ambient},
        .diffuse = {light.diffuse},
        .specular = {light.specular}
    });
}

auto OpenGLRenderer::queue_draw_with_phong(
    const RenderCommand& render_command,
    const glm::mat4& model_matrix,
    const Material& material
) -> void {
    this->draw_queue.emplace_back(
        [this, model_matrix, render_command, material] {
            this->transform_uniform_buffer->set_data(OpenGLUniforms::Transform{
                .model_matrix = model_matrix,
                .view_matrix = this->view_matrix,
                .projection_matrix = this->projection_matrix
            });

            this->phong_shader->bind();
            this->phong_shader->set_uniform(
                "view_position", get_view_position(this->view_matrix)
            );
            this->phong_shader->set_uniform(
                "material.shininess", material.shininess
            );
            this->phong_shader->set_uniform("is_cell_shading_enabled", 0);
            render_command(*this);
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
            this->transform_uniform_buffer->set_data(OpenGLUniforms::Transform{
                .model_matrix = model_matrix,
                .view_matrix = this->view_matrix,
                .projection_matrix = this->projection_matrix
            });

            this->phong_shader->bind();
            this->phong_shader->set_uniform(
                "view_position", get_view_position(this->view_matrix)
            );
            this->phong_shader->set_uniform(
                "material.shininess", material.shininess
            );
            this->phong_shader->set_uniform("is_cell_shading_enabled", 1);
            this->phong_shader->set_uniform(
                "cell_shading_levels", cell_shading_levels
            );
            render_command(*this);
        }
    );
}

auto OpenGLRenderer::queue_draw_with_color(
    const RenderCommand& render_command,
    const glm::mat4& model_matrix,
    const glm::vec3& color
) -> void {
    this->draw_queue.emplace_back([this, model_matrix, render_command, color] {
        this->transform_uniform_buffer->set_data(OpenGLUniforms::Transform{
            .model_matrix = model_matrix,
            .view_matrix = this->view_matrix,
            .projection_matrix = this->projection_matrix
        });

        this->color_shader->bind();
        this->color_shader->set_uniform("color", color);
        render_command(*this);
    });
}

auto OpenGLRenderer::draw() -> void {
    const auto draw_scene = [this] {
        for (const auto& draw_call : this->draw_queue) {
            draw_call();
        }
    };

    this->low_res_frame_buffer->bind();
    this->clear(BufferBit::ColorDepth);
    glViewport(
        0,
        0,
        this->low_res_frame_buffer->get_width(),
        this->low_res_frame_buffer->get_height()
    );
    draw_scene();
    this->low_res_frame_buffer->unbind();

    this->clear(BufferBit::ColorDepth);
    glViewport(0, 0, this->get_window_width(), this->get_window_height());
    this->low_res_frame_buffer->blit(
        this->get_window_width(), this->get_window_height()
    );

    this->draw_queue.clear();
}

}  // namespace Luminol::Graphics
