#pragma once

#include <Engine/Graphics/Renderer.hpp>
#include <Engine/Graphics/OpenGL/OpenGLShader.hpp>
#include <Engine/Graphics/OpenGL/OpenGLUniformBuffer.hpp>

namespace Luminol::Graphics {

struct Transform {
    glm::mat4 model_matrix = {1.0f};       // 64 bytes
    glm::mat4 view_matrix = {1.0f};        // 64 bytes
    glm::mat4 projection_matrix = {1.0f};  // 64 bytes
};

struct PaddedVec3 {
    glm::vec3 data = {0.0f, 0.0f, 0.0f};
    float padding = 0.0f;
};

struct Light {
    PaddedVec3 position = {.data = {0.0f, 0.0f, 0.0f}};  // 16 bytes
    glm::vec3 color = {1.0f, 1.0f, 1.0f};                // 12 bytes
    float ambient_intensity = {1.0f};                    // 4 bytes
};

class OpenGLRenderer : public Renderer {
public:
    OpenGLRenderer(Window& window);

    auto set_view_matrix(const glm::mat4& view_matrix) -> void override;
    auto set_projection_matrix(const glm::mat4& projection_matrix)
        -> void override;

    auto clear_color(const glm::vec4& color) const -> void override;
    auto clear(BufferBit buffer_bit) const -> void override;
    auto draw(
        const RenderCommand& render_command, const glm::mat4& model_matrix
    ) const -> void override;

private:
    std::unique_ptr<OpenGLShader> shader = {nullptr};
    std::unique_ptr<OpenGLUniformBuffer<Transform>> transform_uniform_buffer = {
        nullptr
    };
    std::unique_ptr<OpenGLUniformBuffer<Light>> light_uniform_buffer = {nullptr
    };

    glm::mat4 view_matrix = {1.0f};
    glm::mat4 projection_matrix = {1.0f};
};

}  // namespace Luminol::Graphics
