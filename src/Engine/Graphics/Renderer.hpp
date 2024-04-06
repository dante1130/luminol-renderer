#pragma once

#include <glm/glm.hpp>

#include <Engine/Window/Window.hpp>
#include <Engine/Graphics/BufferBit.hpp>
#include <Engine/Graphics/Light.hpp>

namespace Luminol::Graphics {

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
struct Material {
    float shininess = 32.0f;
};
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)

class Renderer;

using RenderCommand = std::function<void()>;

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
    virtual auto queue_draw_with_cell_shading(
        const RenderCommand& render_command,
        const glm::mat4& model_matrix,
        const Material& material,
        float cell_shading_levels
    ) -> void = 0;
    virtual auto queue_draw_with_color(
        const RenderCommand& render_command,
        const glm::mat4& model_matrix,
        const glm::vec3& color
    ) -> void = 0;
    virtual auto draw() -> void = 0;
};

}  // namespace Luminol::Graphics
