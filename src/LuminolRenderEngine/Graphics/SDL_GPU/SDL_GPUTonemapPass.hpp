#pragma once

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUGraphicsPipeline.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUShader.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTexture.hpp>

struct SDL_Window;

namespace Luminol::Graphics::SDL_GPU {

class GPUDevice;
class CommandBuffer;
class RenderPass;

// Resolves the offscreen HDR color target produced by SDL_GPUMeshRenderPass
// into the swapchain: applies Reinhard tonemapping and gamma correction, the
// same operator used by OpenGLHDRRenderPass. Mirrors the fullscreen-pass shape
// used by SDL_GPUAmbientOcclusionPass, but has a single stage and writes
// straight into an already-active render pass owned by the caller.
class SDL_GPUTonemapPass {
public:
    SDL_GPUTonemapPass(GPUDevice& device, SDL_Window* window);

    auto draw(
        CommandBuffer& command_buffer,
        RenderPass& render_pass,
        const Texture& hdr_color_texture,
        float exposure
    ) const -> void;

private:
    Shader fullscreen_vertex_shader;
    Shader tonemap_fragment_shader;
    GraphicsPipeline tonemap_pipeline;

    Sampler clamp_sampler;
};

}  // namespace Luminol::Graphics::SDL_GPU
