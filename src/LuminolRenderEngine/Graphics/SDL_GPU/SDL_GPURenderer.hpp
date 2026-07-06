#pragma once

#include <memory>
#include <vector>

#include <LuminolMaths/Vector.hpp>

#include <LuminolRenderEngine/Graphics/Renderer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUDevice.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUGraphicsPipeline.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUShader.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTexture.hpp>

namespace Luminol::Graphics::SDL_GPU {

struct QueuedDraw {
    RenderableId renderable_id;
    Maths::Matrix4x4f model_matrix;
};

class SDL_GPURenderer : public Renderer {
public:
    SDL_GPURenderer(
        Window& window,
        std::shared_ptr<GraphicsFactory> graphics_factory,
        std::shared_ptr<GPUDevice> gpu_device
    );

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

    std::shared_ptr<GPUDevice> gpu_device;

    Shader mesh_vertex_shader;
    Shader mesh_fragment_shader;
    GraphicsPipeline mesh_pipeline;

    Texture depth_texture;

    std::vector<QueuedDraw> queued_draws;

    Maths::Matrix4x4f view_matrix = Maths::Matrix4x4f::identity();
    Maths::Matrix4x4f projection_matrix = Maths::Matrix4x4f::identity();

    mutable Maths::Vector4f clear_color_value = {0.0F, 0.0F, 0.0F, 1.0F};
    bool luminance_heatmap_enabled = false;
};

}  // namespace Luminol::Graphics::SDL_GPU
