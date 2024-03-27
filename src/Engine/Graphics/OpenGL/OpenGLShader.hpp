#pragma once

#include <Engine/Graphics/Shader.hpp>

namespace Luminol::Graphics {

class OpenGLShader : public Shader {
public:
    OpenGLShader(const OpenGLShader &) = default;
    ~OpenGLShader() override;
    OpenGLShader(OpenGLShader &&) = delete;
    auto operator=(const OpenGLShader &) -> OpenGLShader & = default;
    auto operator=(OpenGLShader &&) -> OpenGLShader & = delete;
    OpenGLShader(const ShaderPaths &paths);

    auto bind() const -> void override;
    auto unbind() const -> void override;

private:
    uint32_t shader_program_id = 0;
};

}  // namespace Luminol::Graphics
