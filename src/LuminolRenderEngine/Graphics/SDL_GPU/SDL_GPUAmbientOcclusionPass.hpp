#pragma once

#include <cstdint>

#include <gsl/gsl>
#include <LuminolMaths/Matrix.hpp>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUGraphicsPipeline.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUInstanceBufferCache.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUMeshRenderPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUShader.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTexture.hpp>
#include <LuminolRenderEngine/Utilities/PerformanceLogger.hpp>

struct SDL_Window;

namespace Luminol::Graphics::SDL_GPU {

class GPUDevice;
class CommandBuffer;
class SDL_GPUFactory;

// Produces a screen-space ambient occlusion texture each frame: a normal
// prepass (writes view-space normals and depth), an SSAO pass (samples
// depth + normals to compute raw occlusion), and a blur pass (removes
// kernel noise). The final blurred AO texture is consumed by the PBR mesh
// pass as an extra fragment sampler.
class SDL_GPUAmbientOcclusionPass {
public:
    SDL_GPUAmbientOcclusionPass(GPUDevice& device, SDL_Window* window);

    auto resize(GPUDevice& device, uint32_t width, uint32_t height) -> void;

    auto draw(
        const SDL_GPUFactory& graphics_factory,
        CommandBuffer& command_buffer,
        const SDL_GPUInstanceBufferCache& instance_buffer_cache,
        gsl::span<const InstanceBatch> instance_batches,
        const Maths::Matrix4x4f& view_matrix,
        const Maths::Matrix4x4f& projection_matrix,
        const Texture& depth_texture,
        Utilities::PerformanceLogger& performance_logger
    ) -> void;

    [[nodiscard]] auto get_ao_texture() const -> const Texture&;
    [[nodiscard]] auto get_sampler() const -> const Sampler&;

private:
    Shader normal_prepass_vertex_shader;
    Shader normal_prepass_fragment_shader;
    GraphicsPipeline normal_prepass_pipeline;

    Shader fullscreen_vertex_shader;
    Shader ssao_fragment_shader;
    GraphicsPipeline ssao_pipeline;
    Shader blur_fragment_shader;
    GraphicsPipeline blur_pipeline;

    Texture normal_texture;
    Texture ssao_raw_texture;
    Texture ssao_texture;

    Sampler clamp_sampler;

    static constexpr auto default_radius = 0.5F;
    static constexpr auto default_bias = 0.025F;
    static constexpr auto default_power = 1.0F;

    float radius = default_radius;
    float bias = default_bias;
    float power = default_power;
};

}  // namespace Luminol::Graphics::SDL_GPU
