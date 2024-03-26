#include "OpenGLRenderer.hpp"

#include <iostream>
#include <fstream>
#include <array>
#include <filesystem>

#include <glad/gl.h>

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
    glUseProgram(drawable.shader_program_id);
    glBindVertexArray(drawable.vertex_array_id);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

auto OpenGLRenderer::test_draw() const -> Drawable {
    uint32_t vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
    assert(vertex_shader_id != 0 && "Failed to create vertex shader");

    const auto vertex_shader_path =
        std::filesystem::path{"res/Shaders/default_vert.glsl"};

    const auto vertex_shader_source = [&vertex_shader_path] {
        auto file = std::ifstream{vertex_shader_path};
        return std::string{
            std::istreambuf_iterator<char>{file},
            std::istreambuf_iterator<char>{}
        };
    }();

    const auto* const vertex_shader_source_c_str = vertex_shader_source.c_str();

    glShaderSource(vertex_shader_id, 1, &vertex_shader_source_c_str, nullptr);
    glCompileShader(vertex_shader_id);

    int32_t compile_status = {0};
    glGetShaderiv(vertex_shader_id, GL_COMPILE_STATUS, &compile_status);
    if (compile_status == GL_FALSE) {
        int32_t log_length = {0};
        glGetShaderiv(vertex_shader_id, GL_INFO_LOG_LENGTH, &log_length);

        auto log = std::string(log_length, '\0');
        glGetShaderInfoLog(vertex_shader_id, log_length, nullptr, log.data());

        std::cerr << "Vertex shader compilation failed: " << log << '\n';
        assert(false);
    }

    uint32_t fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
    assert(fragment_shader_id != 0 && "Failed to create fragment shader");

    const auto fragment_shader_path =
        std::filesystem::path{"res/Shaders/default_frag.glsl"};

    const auto fragment_shader_source = [&fragment_shader_path] {
        auto file = std::ifstream{fragment_shader_path};
        return std::string{
            std::istreambuf_iterator<char>{file},
            std::istreambuf_iterator<char>{}
        };
    }();

    const auto* const fragment_shader_source_c_str =
        fragment_shader_source.c_str();

    glShaderSource(
        fragment_shader_id, 1, &fragment_shader_source_c_str, nullptr
    );
    glCompileShader(fragment_shader_id);

    glGetShaderiv(fragment_shader_id, GL_COMPILE_STATUS, &compile_status);
    if (compile_status == GL_FALSE) {
        int32_t log_length = {0};
        glGetShaderiv(fragment_shader_id, GL_INFO_LOG_LENGTH, &log_length);

        auto log = std::string(log_length, '\0');
        glGetShaderInfoLog(fragment_shader_id, log_length, nullptr, log.data());

        std::cerr << "Fragment shader compilation failed: " << log << '\n';
        assert(false);
    }

    uint32_t shader_program_id = glCreateProgram();

    glAttachShader(shader_program_id, vertex_shader_id);
    glAttachShader(shader_program_id, fragment_shader_id);
    glLinkProgram(shader_program_id);

    int32_t link_status = {0};
    glGetProgramiv(shader_program_id, GL_LINK_STATUS, &link_status);
    if (link_status == GL_FALSE) {
        int32_t log_length = {0};
        glGetProgramiv(shader_program_id, GL_INFO_LOG_LENGTH, &log_length);

        auto log = std::string(log_length, '\0');
        glGetProgramInfoLog(shader_program_id, log_length, nullptr, log.data());

        std::cerr << "Shader program linking failed: " << log << '\n';
        assert(false);
    }

    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);

    constexpr auto vertices = std::array{
        -0.5f,
        -0.5f,
        0.0f,  // bottom left
        0.5f,
        -0.5f,
        0.0f,  // bottom right
        0.0f,
        0.5f,
        0.0f,  // top
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

    return {vertex_array_id, shader_program_id};
}

}  // namespace Luminol::Graphics
