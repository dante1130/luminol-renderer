#pragma once

#include <glm/glm.hpp>

#include <Engine/Window/Window.hpp>
#include <Engine/Graphics/BufferBit.hpp>

namespace Luminol::Graphics {

struct Light {
    glm::vec3 position = {0.0f, 0.0f, 0.0f};
    glm::vec3 color = {1.0f, 1.0f, 1.0f};
    float ambient_intensity = 1.0f;
    float specular_intensity = 1.0f;
};

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
struct Material {
    glm::vec3 ambient = {1.0f, 1.0f, 1.0f};
    glm::vec3 diffuse = {1.0f, 1.0f, 1.0f};
    glm::vec3 specular = {1.0f, 1.0f, 1.0f};
    float shininess = 32.0f;
};
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)

class Renderer;

using RenderCommand = std::function<void(const Renderer&)>;

class Renderer {
public:
    Renderer() = default;
    virtual ~Renderer() = default;
    Renderer(const Renderer&) = default;
    Renderer(Renderer&&) = delete;
    auto operator=(const Renderer&) -> Renderer& = default;
    auto operator=(Renderer&&) -> Renderer& = delete;

    virtual auto set_view_matrix(const glm::mat4& view_matrix) -> void = 0;
    virtual auto set_projection_matrix(const glm::mat4& projection_matrix)
        -> void = 0;

    virtual auto clear_color(const glm::vec4& color) const -> void = 0;
    virtual auto clear(BufferBit buffer_bit) const -> void = 0;
    virtual auto update_light(const Light& light) -> void = 0;
    virtual auto queue_draw_with_phong(
        const RenderCommand& render_command,
        const glm::mat4& model_matrix,
        const Material& material
    ) -> void = 0;
    virtual auto queue_draw_with_color(
        const RenderCommand& render_command,
        const glm::mat4& model_matrix,
        const glm::vec3& color
    ) -> void = 0;
    virtual auto draw() -> void = 0;
};

}  // namespace Luminol::Graphics
