#include "OpenGLShader.hpp"

#include <fstream>
#include <iostream>

#include <gsl/gsl>
#include <glad/gl.h>

namespace {

auto get_shader_info_log(uint32_t shader_id) -> std::string {
    int32_t log_length = {0};
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);

    auto info_log = std::string(log_length, '\0');
    glGetShaderInfoLog(shader_id, log_length, nullptr, info_log.data());

    return info_log;
}

auto get_program_info_log(uint32_t program_id) -> std::string {
    int32_t log_length = {0};
    glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);

    auto info_log = std::string(log_length, '\0');
    glGetProgramInfoLog(program_id, log_length, nullptr, info_log.data());

    return info_log;
}

auto compile_shader(const std::string& source, GLenum type) -> uint32_t {
    const auto shader_id = glCreateShader(type);
    const auto* const source_ptr = source.c_str();
    glShaderSource(shader_id, 1, &source_ptr, nullptr);
    glCompileShader(shader_id);

    int32_t compile_status = 0;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compile_status);

    if (compile_status == GL_FALSE) {
        const auto info_log = get_shader_info_log(shader_id);
        std::cerr << "Shader compilation failed: " << info_log << '\n';
        return 0;
    }

    return shader_id;
}

auto read_and_compile_shader(const std::filesystem::path& path, GLenum type)
    -> uint32_t {
    auto file = std::ifstream{path};
    const auto source = std::string{
        std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}
    };

    const auto shader_id = compile_shader(source, type);

    Ensures(shader_id != 0);

    return shader_id;
}

auto link_program(uint32_t program_id) -> void {
    glLinkProgram(program_id);

    int32_t link_status = 0;
    glGetProgramiv(program_id, GL_LINK_STATUS, &link_status);

    if (link_status == GL_FALSE) {
        const auto info_log = get_program_info_log(program_id);
        std::cerr << "Shader program linking failed: " << info_log << '\n';
        Ensures(false);
    }
}

auto validate_program(uint32_t program_id) -> void {
    glValidateProgram(program_id);

    int32_t validate_status = 0;
    glGetProgramiv(program_id, GL_VALIDATE_STATUS, &validate_status);

    if (validate_status == GL_FALSE) {
        const auto info_log = get_program_info_log(program_id);
        std::cerr << "Shader program validation failed: " << info_log << '\n';
        Ensures(false);
    }
}

}  // namespace

namespace Luminol::Graphics {

OpenGLShader::OpenGLShader(const ShaderPaths& paths)
    : shader_program_id(glCreateProgram()) {
    Expects(this->shader_program_id != 0);

    if (paths.vertex_shader_path.has_value()) {
        const auto vertex_shader_id = read_and_compile_shader(
            paths.vertex_shader_path.value(), GL_VERTEX_SHADER
        );
        glAttachShader(this->shader_program_id, vertex_shader_id);
        glDeleteShader(vertex_shader_id);
    }

    if (paths.fragment_shader_path.has_value()) {
        const auto fragment_shader_id = read_and_compile_shader(
            paths.fragment_shader_path.value(), GL_FRAGMENT_SHADER
        );
        glAttachShader(this->shader_program_id, fragment_shader_id);
        glDeleteShader(fragment_shader_id);
    }

    if (paths.geometry_shader_path.has_value()) {
        const auto geometry_shader_id = read_and_compile_shader(
            paths.geometry_shader_path.value(), GL_GEOMETRY_SHADER
        );
        glAttachShader(this->shader_program_id, geometry_shader_id);
        glDeleteShader(geometry_shader_id);
    }

    if (paths.tessellation_control_shader_path.has_value()) {
        const auto tessellation_control_shader_id = read_and_compile_shader(
            paths.tessellation_control_shader_path.value(),
            GL_TESS_CONTROL_SHADER
        );
        glAttachShader(this->shader_program_id, tessellation_control_shader_id);
        glDeleteShader(tessellation_control_shader_id);
    }

    if (paths.tessellation_evaluation_shader_path.has_value()) {
        const auto tessellation_evaluation_shader_id = read_and_compile_shader(
            paths.tessellation_evaluation_shader_path.value(),
            GL_TESS_EVALUATION_SHADER
        );
        glAttachShader(
            this->shader_program_id, tessellation_evaluation_shader_id
        );
        glDeleteShader(tessellation_evaluation_shader_id);
    }

    if (paths.compute_shader_path.has_value()) {
        const auto compute_shader_id = read_and_compile_shader(
            paths.compute_shader_path.value(), GL_COMPUTE_SHADER
        );
        glAttachShader(this->shader_program_id, compute_shader_id);
        glDeleteShader(compute_shader_id);
    }

    link_program(this->shader_program_id);
    validate_program(this->shader_program_id);
}

OpenGLShader::~OpenGLShader() { glDeleteProgram(this->shader_program_id); }

auto OpenGLShader::bind() const -> void {
    glUseProgram(this->shader_program_id);
}

// NOLINTBEGIN(readability-convert-member-functions-to-static)
auto OpenGLShader::unbind() const -> void { glUseProgram(0); }
// NOLINTEND(readability-convert-member-functions-to-static)

auto OpenGLShader::set_uniform_block_binding_point(
    const std::string& block_name, uint32_t binding_point
) const -> void {
    const auto block_index =
        glGetUniformBlockIndex(this->shader_program_id, block_name.c_str());
    glUniformBlockBinding(this->shader_program_id, block_index, binding_point);
}

auto OpenGLShader::set_uniform(const std::string& name, const glm::mat4& matrix)
    const -> void {
    const auto location =
        glGetUniformLocation(this->shader_program_id, name.c_str());
    glUniformMatrix4fv(location, 1, GL_FALSE, &matrix[0][0]);
}

auto OpenGLShader::set_uniform(const std::string& name, const glm::vec4& vector)
    const -> void {
    const auto location =
        glGetUniformLocation(this->shader_program_id, name.c_str());
    glUniform4fv(location, 1, &vector[0]);
}

auto OpenGLShader::set_uniform(const std::string& name, const glm::vec3& vector)
    const -> void {
    const auto location =
        glGetUniformLocation(this->shader_program_id, name.c_str());
    glUniform3fv(location, 1, &vector[0]);
}

auto OpenGLShader::set_uniform(const std::string& name, const glm::vec2& vector)
    const -> void {
    const auto location =
        glGetUniformLocation(this->shader_program_id, name.c_str());
    glUniform2fv(location, 1, &vector[0]);
}

auto OpenGLShader::set_uniform(const std::string& name, float value) const
    -> void {
    const auto location =
        glGetUniformLocation(this->shader_program_id, name.c_str());
    glUniform1f(location, value);
}

auto OpenGLShader::set_uniform(const std::string& name, int value) const
    -> void {
    const auto location =
        glGetUniformLocation(this->shader_program_id, name.c_str());
    glUniform1i(location, value);
}

}  // namespace Luminol::Graphics
