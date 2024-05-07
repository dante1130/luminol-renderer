#pragma once

#include <gsl/gsl>
#include <glm/glm.hpp>

#include <LuminolRenderEngine/Graphics/Renderable.hpp>
#include <LuminolRenderEngine/Window/Window.hpp>
#include <LuminolRenderEngine/Graphics/BufferBit.hpp>
#include <LuminolRenderEngine/Graphics/LightManager.hpp>

namespace Luminol::Graphics {

class Renderer {
public:
    Renderer() = default;
    virtual ~Renderer() = default;
    Renderer(const Renderer&) = default;
    Renderer(Renderer&&) = delete;
    auto operator=(const Renderer&) -> Renderer& = default;
    auto operator=(Renderer&&) -> Renderer& = delete;

    auto get_light_manager() -> LightManager&;

    virtual auto set_view_matrix(const glm::mat4& view_matrix) -> void = 0;
    virtual auto set_projection_matrix(const glm::mat4& projection_matrix)
        -> void = 0;

    virtual auto set_exposure(float exposure) -> void = 0;

    virtual auto clear_color(const glm::vec4& color) const -> void = 0;
    virtual auto clear(BufferBit buffer_bit) const -> void = 0;

    virtual auto queue_draw(
        const Renderable& renderable, const glm::mat4& model_matrix
    ) -> void = 0;

    virtual auto queue_draw_with_color(
        const Renderable& renderable,
        const glm::mat4& model_matrix,
        const glm::vec3& color
    ) -> void = 0;
    virtual auto queue_draw_with_color_instanced(
        const Renderable& renderable,
        gsl::span<glm::mat4> model_matrices,
        gsl::span<glm::vec3> colors
    ) -> void = 0;

    virtual auto draw() -> void = 0;

private:
    LightManager light_manager;
};

}  // namespace Luminol::Graphics
