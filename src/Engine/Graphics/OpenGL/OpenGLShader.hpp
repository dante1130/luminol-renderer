#pragma once

#include <glm/glm.hpp>

#include <Engine/Graphics/Shader.hpp>

namespace Luminol::Graphics {

class OpenGLShader : public Shader {
public:
    OpenGLShader() = default;
    OpenGLShader(const ShaderPaths &paths);
    ~OpenGLShader() override;
    OpenGLShader(const OpenGLShader &) = delete;
    OpenGLShader(OpenGLShader &&) = default;
    auto operator=(const OpenGLShader &) -> OpenGLShader & = delete;
    auto operator=(OpenGLShader &&) -> OpenGLShader & = default;

    auto bind() const -> void override;
    auto unbind() const -> void override;

    auto set_uniform(const std::string &name, const glm::mat4 &matrix) const
        -> void;
    auto set_uniform(const std::string &name, const glm::vec4 &vector) const
        -> void;
    auto set_uniform(const std::string &name, const glm::vec3 &vector) const
        -> void;
    auto set_uniform(const std::string &name, const glm::vec2 &vector) const
        -> void;
    auto set_uniform(const std::string &name, float value) const -> void;
    auto set_uniform(const std::string &name, int value) const -> void;

private:
    uint32_t shader_program_id = 0;
};

}  // namespace Luminol::Graphics
