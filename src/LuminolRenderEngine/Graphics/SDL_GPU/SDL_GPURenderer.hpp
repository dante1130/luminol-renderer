#pragma once

#include <LuminolMaths/Vector.hpp>

#include <LuminolRenderEngine/Graphics/Renderer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUDevice.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUGraphicsPipeline.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUShader.hpp>

namespace Luminol::Graphics::SDL_GPU {

class SDL_GPURenderer : public Renderer {
public:
    SDL_GPURenderer(Window& window, GraphicsApi graphics_api);

    auto set_view_matrix(const Maths::Matrix4x4f& view_matrix) -> void override;
    auto set_projection_matrix(const Maths::Matrix4x4f& projection_matrix)
        -> void override;

    auto set_exposure(float exposure) -> void override;

    auto set_luminance_heatmap_enabled(bool enabled) -> void override;
    [[nodiscard]] auto get_luminance_heatmap_enabled() const -> bool override;

    auto clear_color(const Maths::Vector4f& color) const -> void override;
    auto clear(BufferBit buffer_bit) const -> void override;

    auto queue_draw(
        RenderableId renderable_id, const Maths::Matrix4x4f& model_matrix
    ) -> void override;

    auto queue_draw_with_color(
        RenderableId renderable_id,
        const Maths::Matrix4x4f& model_matrix,
        const Maths::Vector3f& color
    ) -> void override;

    auto queue_draw_line(
        const Maths::Vector3f& start,
        const Maths::Vector3f& end,
        const Maths::Vector3f& color
    ) -> void override;

    auto draw() -> void override;

private:
    SDL_Window* sdl_window = nullptr;

    std::function<int32_t()> get_window_width;
    std::function<int32_t()> get_window_height;

    GPUDevice gpu_device;

    Shader triangle_vertex_shader;
    Shader triangle_fragment_shader;
    GraphicsPipeline triangle_pipeline;

    mutable Maths::Vector4f clear_color_value = {0.0F, 0.0F, 0.0F, 1.0F};
    bool luminance_heatmap_enabled = false;
};

}  // namespace Luminol::Graphics::SDL_GPU
