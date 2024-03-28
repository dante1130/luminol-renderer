#pragma once

#include <filesystem>

namespace Luminol::Graphics {

struct ShaderPaths {
    std::optional<std::filesystem::path> vertex_shader_path;
    std::optional<std::filesystem::path> fragment_shader_path;
    std::optional<std::filesystem::path> geometry_shader_path;
    std::optional<std::filesystem::path> tessellation_control_shader_path;
    std::optional<std::filesystem::path> tessellation_evaluation_shader_path;
    std::optional<std::filesystem::path> compute_shader_path;
};

class Shader {
public:
    Shader(const ShaderPaths& paths);
    virtual ~Shader() = default;
    Shader(const Shader&) = delete;
    Shader(Shader&&) = default;
    auto operator=(const Shader&) -> Shader& = delete;
    auto operator=(Shader&&) -> Shader& = default;

    virtual auto bind() const -> void = 0;
    virtual auto unbind() const -> void = 0;
};

}  // namespace Luminol::Graphics
