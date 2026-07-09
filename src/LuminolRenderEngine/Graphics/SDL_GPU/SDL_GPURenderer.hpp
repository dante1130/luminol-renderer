#pragma once

#include <memory>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <LuminolMaths/Vector.hpp>

#include <LuminolRenderEngine/Graphics/Renderer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUAmbientOcclusionPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUDevice.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUFont.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUMeshRenderPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUShadowPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTextRenderPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTexture.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTonemapPass.hpp>
#include <LuminolRenderEngine/Utilities/PerformanceLogger.hpp>

namespace Luminol::Graphics::SDL_GPU {

class SDL_GPUFactory;

class SDL_GPURenderer : public Renderer {
public:
    SDL_GPURenderer(
        Window& window,
        std::shared_ptr<SDL_GPUFactory> graphics_factory,
        std::shared_ptr<GPUDevice> gpu_device,
        SampleCount requested_msaa_sample_count = SampleCount::x4
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

    auto queue_draw_instanced(
        RenderableId renderable_id,
        gsl::span<const Maths::Matrix4x4f> model_matrices
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

    auto queue_draw_text(
        FontId font_id,
        std::string_view text,
        const Maths::Vector2f& position,
        const Maths::Vector4f& color
    ) -> void override;

    auto draw() -> void override;

private:
    SDL_Window* sdl_window = nullptr;

    std::shared_ptr<SDL_GPUFactory> sdl_gpu_factory;
    std::shared_ptr<GPUDevice> gpu_device;

    SDL_GPUMeshRenderPass mesh_render_pass;
    SDL_GPUAmbientOcclusionPass ao_pass;
    SDL_GPUShadowPass shadow_pass;
    SDL_GPUTonemapPass tonemap_pass;
    SDL_GPUTextRenderPass text_render_pass;

    Texture depth_texture;
    Texture hdr_color_texture;

    // Dedicated multisampled targets for the main forward pass only,
    // resolved into hdr_color_texture at the end of the pass. Decoupled from
    // the AO prepass's single-sample depth_texture, since SDL_GPU requires
    // every attachment in a render pass to share one sample count.
    SampleCount msaa_sample_count;
    Texture msaa_color_texture;
    Texture msaa_depth_texture;

    Utilities::PerformanceLogger performance_logger;

    std::unordered_map<RenderableId, std::vector<Maths::Matrix4x4f>>
        queued_draws;

    Maths::Matrix4x4f view_matrix = Maths::Matrix4x4f::identity();
    Maths::Matrix4x4f projection_matrix = Maths::Matrix4x4f::identity();

    mutable Maths::Vector4f clear_color_value = {0.0F, 0.0F, 0.0F, 1.0F};
    bool luminance_heatmap_enabled = false;
    float exposure = 1.0F;
};

}  // namespace Luminol::Graphics::SDL_GPU
