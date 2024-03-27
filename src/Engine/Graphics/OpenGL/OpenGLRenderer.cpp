#include "OpenGLRenderer.hpp"

#include <iostream>
#include <array>
#include <filesystem>

#include <gsl/gsl>
#include <glad/gl.h>

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

}  // namespace

namespace Luminol::Graphics {

OpenGLRenderer::OpenGLRenderer(const Window& window) {
    const auto version = gladLoadGL(window.get_proc_address());
    assert(version != 0);

    std::cout << "OpenGL Version " << GLAD_VERSION_MAJOR(version) << "."
              << GLAD_VERSION_MINOR(version) << " loaded\n";
}

auto OpenGLRenderer::clear_color(const glm::vec4& color) const -> void {
    glClearColor(color.r, color.g, color.b, color.a);
}

auto OpenGLRenderer::clear(BufferBit buffer_bit) const -> void {
    glClear(buffer_bit_to_gl(buffer_bit));
}

auto OpenGLRenderer::draw(const Drawable& drawable) const -> void {
    drawable.shader->bind();
    glBindVertexArray(drawable.vertex_array_id);
    glDrawArrays(GL_TRIANGLES, 0, drawable.vertex_count);
}

auto OpenGLRenderer::test_draw() const -> Drawable {
    const auto shader_source_paths = ShaderPaths{
        .vertex_shader_path =
            std::filesystem::path{"res/Shaders/default_vert.glsl"},
        .fragment_shader_path =
            std::filesystem::path{"res/Shaders/default_frag.glsl"}
    };

    auto shader = std::unique_ptr<Shader>{
        std::make_unique<OpenGLShader>(shader_source_paths)
    };

    constexpr auto vertices = std::array{
        glm::vec3(-0.5f, -0.5f, 0.0f),  // bottom left
        glm::vec3(0.5f, -0.5f, 0.0f),   // bottom right
        glm::vec3(0.0f, 0.5f, 0.0f)     // top
    };

    uint32_t vertex_buffer_id = {0};
    glGenBuffers(1, &vertex_buffer_id);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_id);
    glNamedBufferData(
        vertex_buffer_id, sizeof(vertices), vertices.data(), GL_STATIC_DRAW
    );

    uint32_t vertex_array_id = {0};
    glGenVertexArrays(1, &vertex_array_id);
    glBindVertexArray(vertex_array_id);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    return Drawable{
        .vertex_array_id = vertex_array_id,
        .shader = std::move(shader),
        .vertex_count = gsl::narrow_cast<int32_t>(vertices.size())
    };
}

}  // namespace Luminol::Graphics
