#pragma once

#include <cstdint>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUComputePipeline.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUGraphicsPipeline.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUShader.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTexture.hpp>

struct SDL_Window;

namespace Luminol::Graphics::SDL_GPU {

class GPUDevice;
class CommandBuffer;

// Builds a Hi-Z (max-depth) mip pyramid from the previous frame's depth
// texture, for reprojected-prior-frame occlusion culling in
// SDL_GPUInstanceCullPass. Must run before any other render/compute pass
// touches the depth texture this frame - see SDL_GPURenderer::draw, which
// calls build() while depth_texture still holds last frame's contents,
// before ao_pass.draw() overwrites it with this frame's geometry.
class SDL_GPUHiZPass {
public:
    SDL_GPUHiZPass(GPUDevice& device, SDL_Window* window);

    auto resize(GPUDevice& device, uint32_t width, uint32_t height) -> void;

    auto build(
        CommandBuffer& command_buffer,
        const Texture& previous_frame_depth_texture,
        const Sampler& point_sampler
    ) -> void;

    [[nodiscard]] auto get_pyramid_texture() const -> const Texture&;
    [[nodiscard]] auto get_pyramid_sampler() const -> const Sampler&;
    [[nodiscard]] auto get_mip_levels() const -> uint32_t;

private:
    Shader fullscreen_vertex_shader;
    Shader copy_depth_fragment_shader;
    GraphicsPipeline copy_depth_pipeline;

    ComputePipeline downsample_pipeline;

    Texture pyramid_texture;
    Sampler pyramid_sampler;
};

}  // namespace Luminol::Graphics::SDL_GPU
