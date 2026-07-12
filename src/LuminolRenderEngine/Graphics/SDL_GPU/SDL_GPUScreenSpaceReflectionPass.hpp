#pragma once

#include <cstdint>

#include <LuminolMaths/Matrix.hpp>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUGraphicsPipeline.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUShader.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTexture.hpp>
#include <LuminolRenderEngine/Utilities/PerformanceLogger.hpp>

struct SDL_Window;

namespace Luminol::Graphics::SDL_GPU {

class GPUDevice;
class CommandBuffer;

// Screen-space reflections. A fullscreen pass that traces mirror reflection
// rays in view space against this frame's depth buffer (from the AO normal
// prepass) and samples the PREVIOUS frame's resolved HDR color on a hit. The
// output ssr_texture (rgb = reflected color, a = confidence) is consumed by
// the forward PBR pass, which blends it over the global prefiltered specular
// IBL weighted by that confidence. Modeled on SDL_GPUAmbientOcclusionPass.
class SDL_GPUScreenSpaceReflectionPass {
public:
    SDL_GPUScreenSpaceReflectionPass(GPUDevice& device, SDL_Window* window);

    auto resize(GPUDevice& device, uint32_t width, uint32_t height) -> void;

    auto draw(
        CommandBuffer& command_buffer,
        const Maths::Matrix4x4f& projection_matrix,
        const Texture& depth_texture,
        const Texture& normal_texture,
        const Texture& previous_color_texture,
        bool has_valid_previous_color,
        Utilities::PerformanceLogger& performance_logger
    ) -> void;

    [[nodiscard]] auto get_ssr_texture() const -> const Texture&;
    [[nodiscard]] auto get_sampler() const -> const Sampler&;

private:
    Shader fullscreen_vertex_shader;
    Shader ssr_fragment_shader;
    GraphicsPipeline ssr_pipeline;

    // Confidence-weighted blur that denoises the jittered trace result.
    Shader resolve_fragment_shader;
    GraphicsPipeline resolve_pipeline;

    // Raw trace output; resolved (denoised) output consumed by the forward
    // pass. Kept separate since the resolve pass reads the raw texture while
    // writing the resolved one.
    Texture ssr_texture;
    Texture ssr_resolved_texture;
    Sampler clamp_sampler;

    static constexpr auto default_max_distance = 8.0F;
    // View-space depth tolerance for a hit. Kept tight so the reflection of
    // thin geometry (e.g. a knife blade) doesn't smear along the ray - a large
    // tolerance registers a "hit" anywhere in a thick band behind the front
    // surface, stretching the thin object's colour downward in the reflection.
    static constexpr auto default_thickness = 0.25F;
    // Upper bound on the screen-space march samples. The pass walks ~one pixel
    // per step; a high cap keeps long reflection rays (tall objects, grazing
    // angles) finely sampled so the hit silhouette doesn't staircase.
    static constexpr auto default_max_steps = 256.0F;

    float max_distance = default_max_distance;
    float thickness = default_thickness;
    float max_steps = default_max_steps;
};

}  // namespace Luminol::Graphics::SDL_GPU
