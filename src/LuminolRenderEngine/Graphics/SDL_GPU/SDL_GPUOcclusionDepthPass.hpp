#pragma once

#include <cstdint>

#include <gsl/gsl>
#include <LuminolMaths/Matrix.hpp>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUGraphicsPipeline.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUInstanceBufferCache.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUInstanceCullPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUShader.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTexture.hpp>

struct SDL_Window;

namespace Luminol::Graphics::SDL_GPU {

class GPUDevice;
class CommandBuffer;
class SDL_GPUFactory;
class Buffer;

// Depth-only bootstrap pass for two-phase Hi-Z occlusion culling: draws
// phase-1-culled geometry (indirect, from SDL_GPUInstanceCullPass) with no
// color output, into its own scratch depth texture. Its only purpose is to
// give SDL_GPUHiZPass a same-frame, same-camera depth source to rebuild from
// before phase 2's cull runs - see SDL_GPURenderer::draw. Modeled on
// SDL_GPUShadowPass's depth-only pipeline shape, but consumes culled
// indirect draws (like SDL_GPUAmbientOcclusionPass's normal prepass) instead
// of drawing every instance uncalled.
class SDL_GPUOcclusionDepthPass {
public:
    SDL_GPUOcclusionDepthPass(GPUDevice& device, SDL_Window* window);

    auto resize(GPUDevice& device, uint32_t width, uint32_t height) -> void;

    auto draw(
        const SDL_GPUFactory& graphics_factory,
        CommandBuffer& command_buffer,
        const SDL_GPUInstanceBufferCache& instance_buffer_cache,
        gsl::span<const InstanceBatch> instance_batches,
        const Maths::Matrix4x4f& view_matrix,
        const Maths::Matrix4x4f& projection_matrix,
        const Buffer& indirect_command_buffer,
        const Buffer& visible_instance_indices_buffer,
        const InstanceCullLayout& instance_cull_layout
    ) -> void;

    [[nodiscard]] auto get_depth_texture() const -> const Texture&;

private:
    Shader vertex_shader;
    Shader fragment_shader;
    GraphicsPipeline pipeline;

    Texture depth_texture;
};

}  // namespace Luminol::Graphics::SDL_GPU
