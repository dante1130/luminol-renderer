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

    Texture ssr_texture;
    Sampler clamp_sampler;

    static constexpr auto default_max_distance = 8.0F;
    static constexpr auto default_thickness = 0.5F;
    static constexpr auto default_step_count = 32.0F;

    float max_distance = default_max_distance;
    float thickness = default_thickness;
    float step_count = default_step_count;
};

}  // namespace Luminol::Graphics::SDL_GPU
