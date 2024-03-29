#include "OpenGLRenderer.hpp"

#include <iostream>
#include <array>
#include <filesystem>

#include <gsl/gsl>
#include <glad/gl.h>

#include <Engine/Graphics/OpenGL/OpenGLShader.hpp>
#include <Engine/Graphics/OpenGL/OpenGLVertexArrayObject.hpp>

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

}  // namespace

namespace Luminol::Graphics {

OpenGLRenderer::OpenGLRenderer(const Window& window) {
    const auto version = gladLoadGL(window.get_proc_address());
    assert(version != 0);

    std::cout << "OpenGL Version " << GLAD_VERSION_MAJOR(version) << "."
              << GLAD_VERSION_MINOR(version) << " loaded\n";

    this->shader = std::make_unique<OpenGLShader>(get_default_shader_paths());
}

auto OpenGLRenderer::clear_color(const glm::vec4& color) const -> void {
    glClearColor(color.r, color.g, color.b, color.a);
}

auto OpenGLRenderer::clear(BufferBit buffer_bit) const -> void {
    glClear(buffer_bit_to_gl(buffer_bit));
}

auto OpenGLRenderer::draw(const RenderCommand& render_command) const -> void {
    this->shader->bind();
    render_command(*this);
}

}  // namespace Luminol::Graphics
