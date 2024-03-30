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

auto get_default_shader_paths() -> ShaderPaths {
    return ShaderPaths{
        .vertex_shader_path =
            std::filesystem::path{"res/Shaders/default_vert.glsl"},
        .fragment_shader_path =
            std::filesystem::path{"res/Shaders/default_frag.glsl"}
    };
}

auto get_projection_matrix(int32_t width, int32_t height) -> glm::mat4 {
    constexpr auto fov_degrees = 45.0f;
    constexpr auto near_plane = 0.1f;
    constexpr auto far_plane = 100.0f;

    return glm::perspective(
        glm::radians(fov_degrees),
        static_cast<float>(width) / static_cast<float>(height),
        near_plane,
        far_plane
    );
}

}  // namespace

namespace Luminol::Graphics {

OpenGLRenderer::OpenGLRenderer(const Window& window)
    : get_window_width{[&window]() { return window.get_width(); }},
      get_window_height{[&window]() { return window.get_height(); }} {
    const auto version = gladLoadGL(window.get_proc_address());
    Ensures(version != 0);

    std::cout << "OpenGL Version " << GLAD_VERSION_MAJOR(version) << "."
              << GLAD_VERSION_MINOR(version) << " loaded\n";

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(gl_debug_message_callback, nullptr);

    this->shader = std::make_unique<OpenGLShader>(get_default_shader_paths());
    this->shader->bind();
    this->shader->set_sampler_binding_point(
        "texture_diffuse", SamplerBindingPoint::TextureDiffuse
    );
    this->shader->set_uniform_block_binding_point(
        "Transform", UniformBufferBindingPoint::Transform
    );
    this->shader->unbind();

    this->transform_uniform_buffer =
        std::make_unique<OpenGLUniformBuffer<Transform>>(
            Transform{
                .model_matrix = glm::mat4{1.0f},
                .projection_matrix = get_projection_matrix(
                    this->get_window_width(), this->get_window_height()
                )
            },
            UniformBufferBindingPoint::Transform
        );
}

auto OpenGLRenderer::clear_color(const glm::vec4& color) const -> void {
    glClearColor(color.r, color.g, color.b, color.a);
}

auto OpenGLRenderer::clear(BufferBit buffer_bit) const -> void {
    glClear(buffer_bit_to_gl(buffer_bit));
}

auto OpenGLRenderer::draw(
    const RenderCommand& render_command, const glm::mat4& model_matrix
) const -> void {
    this->transform_uniform_buffer->set_data(Transform{
        .model_matrix = model_matrix,
        .projection_matrix = get_projection_matrix(
            this->get_window_width(), this->get_window_height()
        )
    });

    this->shader->bind();
    render_command(*this);
}

}  // namespace Luminol::Graphics
