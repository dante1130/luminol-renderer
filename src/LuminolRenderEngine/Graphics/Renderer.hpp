#pragma once

#include <glm/glm.hpp>
#include <LuminolMaths/Vector.hpp>

#include <LuminolRenderEngine/Window/Window.hpp>
#include <LuminolRenderEngine/Graphics/Renderable.hpp>
#include <LuminolRenderEngine/Graphics/BufferBit.hpp>
#include <LuminolRenderEngine/Graphics/LightManager.hpp>
#include <LuminolRenderEngine/Graphics/RenderableManager.hpp>

namespace Luminol::Graphics {

class Renderer {
public:
    Renderer(GraphicsApi api);
    virtual ~Renderer() = default;
    Renderer(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    auto operator=(const Renderer&) -> Renderer& = delete;
    auto operator=(Renderer&&) -> Renderer& = delete;

    [[nodiscard]] auto get_renderable_manager() const
        -> const RenderableManager&;
    [[nodiscard]] auto get_renderable_manager() -> RenderableManager&;

    [[nodiscard]] auto get_light_manager() const -> const LightManager&;
    [[nodiscard]] auto get_light_manager() -> LightManager&;

    virtual auto set_view_matrix(const glm::mat4& view_matrix) -> void = 0;
    virtual auto set_projection_matrix(const glm::mat4& projection_matrix)
        -> void = 0;

    virtual auto set_exposure(float exposure) -> void = 0;

    virtual auto clear_color(const Maths::Vector4f& color) const -> void = 0;
    virtual auto clear(BufferBit buffer_bit) const -> void = 0;

    virtual auto queue_draw(
        RenderableId renderable_id, const glm::mat4& model_matrix
    ) -> void = 0;

    virtual auto queue_draw_with_color(
        RenderableId renderable_id,
        const glm::mat4& model_matrix,
        const Maths::Vector3f& color
    ) -> void = 0;

    virtual auto queue_draw_line(
        const Maths::Vector3f& start_position,
        const Maths::Vector3f& end_position,
        const Maths::Vector3f& color
    ) -> void = 0;

    virtual auto draw() -> void = 0;

private:
    RenderableManager renderable_manager;
    LightManager light_manager;
};

}  // namespace Luminol::Graphics
