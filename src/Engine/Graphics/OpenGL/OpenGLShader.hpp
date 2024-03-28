#pragma once

#include <Engine/Graphics/Shader.hpp>

namespace Luminol::Graphics {

class OpenGLShader : public Shader {
public:
    OpenGLShader(const ShaderPaths &paths);
    ~OpenGLShader() override;
    OpenGLShader(const OpenGLShader &) = delete;
    OpenGLShader(OpenGLShader &&) = default;
    auto operator=(const OpenGLShader &) -> OpenGLShader & = delete;
    auto operator=(OpenGLShader &&) -> OpenGLShader & = default;

    auto bind() const -> void override;
    auto unbind() const -> void override;

private:
    uint32_t shader_program_id = 0;
};

}  // namespace Luminol::Graphics
