#pragma once

#include <cstdint>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUGraphicsPipeline.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUShader.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTexture.hpp>

namespace Luminol::Graphics::SDL_GPU {

class GPUDevice;

// Precomputes the three components of split-sum image-based lighting from
// the already-uploaded skybox cubemap: an irradiance cubemap (diffuse IBL),
// a roughness-mipped prefiltered specular cubemap, and an environment-
// independent BRDF LUT. All three are baked once at construction time via a
// dedicated one-shot command buffer (mirrors SDL_GPUSkyboxRenderPass's
// startup-time upload), not recomputed per frame.
class SDL_GPUIBLRenderPass {
public:
    SDL_GPUIBLRenderPass(
        GPUDevice& device,
        const Texture& skybox_texture,
        const Sampler& skybox_sampler
    );

    [[nodiscard]] auto get_irradiance_texture() const -> const Texture&;
    [[nodiscard]] auto get_irradiance_sampler() const -> const Sampler&;

    [[nodiscard]] auto get_prefiltered_texture() const -> const Texture&;
    [[nodiscard]] auto get_prefiltered_sampler() const -> const Sampler&;
    [[nodiscard]] auto get_prefiltered_mip_count() const -> uint32_t;

    [[nodiscard]] auto get_brdf_lut_texture() const -> const Texture&;
    [[nodiscard]] auto get_brdf_lut_sampler() const -> const Sampler&;

private:
    Shader cubemap_face_vertex_shader;
    Shader irradiance_fragment_shader;
    Shader prefilter_fragment_shader;
    GraphicsPipeline irradiance_pipeline;
    GraphicsPipeline prefilter_pipeline;

    Shader fullscreen_vertex_shader;
    Shader brdf_lut_fragment_shader;
    GraphicsPipeline brdf_lut_pipeline;

    Texture irradiance_texture;
    Sampler irradiance_sampler;

    Texture prefiltered_texture;
    Sampler prefiltered_sampler;
    uint32_t prefiltered_mip_count;

    Texture brdf_lut_texture;
    Sampler brdf_lut_sampler;
};

}  // namespace Luminol::Graphics::SDL_GPU
