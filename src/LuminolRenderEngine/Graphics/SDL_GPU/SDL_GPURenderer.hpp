#pragma once

#include <array>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <LuminolMaths/Vector.hpp>

#include <LuminolRenderEngine/Graphics/Renderer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUAmbientOcclusionPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUClusterPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCommandBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUDevice.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUFont.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUHiZPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUIBLRenderPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUInstanceCullPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUMeshRenderPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUOcclusionDepthPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUPointSpotShadowPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUScreenSpaceReflectionPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUShadowPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUSkyboxRenderPass.hpp>
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

    auto clear_color(const Maths::Vector4f& color) const -> void override;

    auto queue_draw(
        RenderableId renderable_id, const Maths::Matrix4x4f& model_matrix
    ) -> void override;

    auto queue_draw_instanced(
        RenderableId renderable_id,
        gsl::span<const Maths::Matrix4x4f> model_matrices
    ) -> void override;

    auto queue_draw_text(
        FontId font_id,
        std::string_view text,
        const Maths::Vector2f& position,
        const Maths::Vector4f& color
    ) -> void override;

    auto draw() -> void override;

    auto set_debug_disable_occlusion_culling(bool disabled) -> void override;
    auto debug_log_visible_instance_count() -> void override;
    auto set_debug_visualize_hiz(bool enabled) -> void override;

private:
    // Recreates all swapchain-resolution-dependent textures/passes when the
    // window has been resized since last frame.
    auto handle_resize(const SwapchainTexture& swapchain) -> void;

    struct FramePrepData {
        std::vector<InstanceBatch> instance_batches;
        std::array<Maths::Vector4f, 6> camera_frustum_planes;
        Maths::Matrix4x4f current_view_projection;
    };
    [[nodiscard]] auto upload_instances_and_compute_frustum(
        CommandBuffer& command_buffer
    ) -> FramePrepData;

    // Two-phase GPU occlusion culling prepass (Hi-Z phase 1 build, phase-1
    // cull, occlusion-depth bootstrap draw, Hi-Z phase 2 build). Must run
    // before any render pass is opened this frame - see the comment on the
    // Hi-Z phase 1 build in the .cpp.
    auto run_occlusion_prepass(
        CommandBuffer& command_buffer,
        gsl::span<const InstanceBatch> instance_batches,
        const std::array<Maths::Vector4f, 6>& camera_frustum_planes,
        const Maths::Matrix4x4f& current_view_projection
    ) -> void;

    // Debug-only short-circuit: blits the Hi-Z pyramid to the swapchain
    // instead of the normal scene. Returns true if the frame was short-
    // circuited, in which case the caller must submit the command buffer,
    // clear queued_draws, and return without recording the rest of the
    // frame.
    [[nodiscard]] auto record_debug_hiz_visualize(
        CommandBuffer& command_buffer, const SwapchainTexture& swapchain
    ) -> bool;

    // AO must run before SSR - SSR consumes AO's same-frame normal texture.
    auto record_ao_and_ssr(
        CommandBuffer& command_buffer,
        gsl::span<const InstanceBatch> instance_batches,
        const InstanceCullLayout& instance_cull_layout
    ) -> void;

    // Cluster light grid/cull, directional cascade shadows, point/spot
    // shadows. Returns this frame's repacked light data (owned by the
    // renderer's LightManager), consumed by record_main_pass afterward.
    [[nodiscard]] auto record_shadows(
        CommandBuffer& command_buffer, gsl::span<const InstanceBatch> instance_batches
    ) -> const Light&;

    // Forward mesh pass followed by the skybox, drawn into the same open
    // render pass.
    auto record_main_pass(
        CommandBuffer& command_buffer,
        const SwapchainTexture& swapchain,
        gsl::span<const InstanceBatch> instance_batches,
        const std::array<Maths::Vector4f, 6>& camera_frustum_planes,
        const InstanceCullLayout& instance_cull_layout,
        const Light& light_manager_data
    ) -> void;

    // Tonemap followed by text, drawn into the same open render pass.
    auto record_tonemap_and_text(
        CommandBuffer& command_buffer, const SwapchainTexture& swapchain
    ) -> void;

    SDL_Window* sdl_window = nullptr;

    std::shared_ptr<SDL_GPUFactory> sdl_gpu_factory;
    std::shared_ptr<GPUDevice> gpu_device;

    SDL_GPUMeshRenderPass mesh_render_pass;
    SDL_GPUAmbientOcclusionPass ao_pass;
    SDL_GPUScreenSpaceReflectionPass ssr_pass;
    SDL_GPUHiZPass hiz_pass;
    // Phase 1: cheap early-out cull against last frame's Hi-Z, current
    // camera. Only feeds occlusion_depth_pass's bootstrap draw - never used
    // for final shading, so any staleness here only costs extra draws in
    // phase 2, never incorrect final visibility.
    SDL_GPUInstanceCullPass phase1_cull_pass;
    SDL_GPUOcclusionDepthPass occlusion_depth_pass;
    // Phase 2: final cull, against a Hi-Z rebuilt from occlusion_depth_pass's
    // same-frame depth - always correct regardless of camera speed/scene
    // density, since there's no cross-frame data in this decision. Feeds
    // every downstream pass (AO, shadows, main pass) exactly as before.
    SDL_GPUInstanceCullPass instance_cull_pass;
    SDL_GPUClusterPass cluster_pass;
    SDL_GPUShadowPass shadow_pass;
    SDL_GPUPointSpotShadowPass point_spot_shadow_pass;
    SDL_GPUTonemapPass tonemap_pass;
    SDL_GPUSkyboxRenderPass skybox_render_pass;
    SDL_GPUIBLRenderPass ibl_render_pass;
    SDL_GPUTextRenderPass text_render_pass;

    Texture depth_texture;
    Texture hdr_color_texture;
    // Last frame's resolved HDR color, used by the SSR pass as its reflection
    // source. Ping-ponged with hdr_color_texture at the end of each frame
    // (std::swap) so no per-frame copy is needed. has_valid_previous_hdr is
    // false on the first frame and immediately after a resize, when this holds
    // no valid data - the SSR pass then produces zero confidence and the
    // forward pass falls back to the global specular IBL.
    Texture previous_hdr_color_texture;
    bool has_valid_previous_hdr = false;
    Sampler point_sampler;

    // Dedicated multisampled targets for the main forward pass only,
    // resolved into hdr_color_texture at the end of the pass. Decoupled from
    // the AO prepass's single-sample depth_texture, since SDL_GPU requires
    // every attachment in a render pass to share one sample count.
    SampleCount msaa_sample_count;
    Texture msaa_color_texture;
    Texture msaa_depth_texture;

    // Debug-only: when true, draw() renders the Hi-Z pyramid's mip 0 to the
    // swapchain instead of the normal scene, to visually sanity-check its
    // contents. See Renderer::set_debug_visualize_hiz.
    Shader hiz_debug_vertex_shader;
    Shader hiz_debug_fragment_shader;
    GraphicsPipeline hiz_debug_pipeline;
    Sampler hiz_debug_sampler;
    bool debug_visualize_hiz = false;

    Utilities::PerformanceLogger performance_logger;

    std::unordered_map<RenderableId, std::vector<Maths::Matrix4x4f>>
        queued_draws;

    Maths::Matrix4x4f view_matrix = Maths::Matrix4x4f::identity();
    Maths::Matrix4x4f projection_matrix = Maths::Matrix4x4f::identity();

    // False on the first frame and immediately after a resize, when
    // depth_texture/hiz_pass hold no valid previous-frame data - disables
    // the occlusion test for that one frame.
    bool has_valid_previous_depth = false;

    // Debug-only: forces the occlusion test off (frustum culling still
    // applies) for A/B comparison while investigating occlusion-culling
    // correctness. See Renderer::set_debug_disable_occlusion_culling.
    bool debug_disable_occlusion_culling = false;

    mutable Maths::Vector4f clear_color_value = {0.0F, 0.0F, 0.0F, 1.0F};
    float exposure = 1.0F;
};

}  // namespace Luminol::Graphics::SDL_GPU
