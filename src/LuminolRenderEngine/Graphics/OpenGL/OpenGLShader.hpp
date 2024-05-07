#pragma once

#include <filesystem>
#include <optional>

#include <glm/glm.hpp>

#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLUniformBindingPoints.hpp>

namespace Luminol::Graphics {

struct ShaderPaths {
    std::optional<std::filesystem::path> vertex_shader_path;
    std::optional<std::filesystem::path> fragment_shader_path;
    std::optional<std::filesystem::path> geometry_shader_path;
    std::optional<std::filesystem::path> tessellation_control_shader_path;
    std::optional<std::filesystem::path> tessellation_evaluation_shader_path;
    std::optional<std::filesystem::path> compute_shader_path;
};

class OpenGLShader {
public:
    OpenGLShader(const ShaderPaths& paths);
    ~OpenGLShader();
    OpenGLShader(const OpenGLShader&) = delete;
    OpenGLShader(OpenGLShader&&) = default;
    auto operator=(const OpenGLShader&) -> OpenGLShader& = delete;
    auto operator=(OpenGLShader&&) -> OpenGLShader& = default;

    auto bind() const -> void;
    auto unbind() const -> void;

    auto set_sampler_binding_point(
        const std::string& sampler_name, SamplerBindingPoint binding_point
    ) const -> void;

    auto set_uniform_block_binding_point(
        const std::string& uniform_block_name,
        UniformBufferBindingPoint binding_point
    ) const -> void;

    auto set_shader_storage_block_binding_point(
        const std::string& shader_storage_block_name,
        ShaderStorageBufferBindingPoint binding_point
    ) const -> void;

    auto set_uniform(const std::string& name, const glm::mat4& matrix) const
        -> void;
    auto set_uniform(const std::string& name, const glm::vec4& vector) const
        -> void;
    auto set_uniform(const std::string& name, const glm::vec3& vector) const
        -> void;
    auto set_uniform(const std::string& name, const glm::vec2& vector) const
        -> void;
    auto set_uniform(const std::string& name, float value) const -> void;
    auto set_uniform(const std::string& name, int value) const -> void;
    auto set_uniform(const std::string& name, bool value) const -> void;

private:
    uint32_t shader_program_id = 0;
};

}  // namespace Luminol::Graphics
