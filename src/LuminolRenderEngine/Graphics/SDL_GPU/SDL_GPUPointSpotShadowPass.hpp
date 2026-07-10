#pragma once

#include <gsl/gsl>
#include <LuminolMaths/Matrix.hpp>
#include <LuminolMaths/Vector.hpp>

#include <LuminolRenderEngine/Graphics/Light.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUGraphicsPipeline.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUInstanceBufferCache.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUMeshRenderPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUShader.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTexture.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTransferBuffer.hpp>
#include <LuminolRenderEngine/Utilities/PerformanceLogger.hpp>

namespace Luminol::Graphics::SDL_GPU {

class GPUDevice;
class CommandBuffer;
class SDL_GPUFactory;

// Renders depth-only shadow maps for a capped, frame-selected subset of
// point and spot lights (see LightManager::update_shadow_casters), since a
// full shadow map per light is infeasible at up to max_point_lights /
// max_spot_lights simultaneous lights. Point lights get a cube face set in a
// TextureCubeArray (one cube per shadow-casting light, addressed by
// direction + array slot); spot lights get a single 2D layer in a
// Texture2DArray. Mirrors the per-pass class shape used by
// SDL_GPUShadowPass.
class SDL_GPUPointSpotShadowPass {
public:
    explicit SDL_GPUPointSpotShadowPass(GPUDevice& device);

    // light_data must already have shadow slots assigned (i.e.
    // LightManager::update_shadow_casters was called before
    // LightManager::get_light_data() this frame).
    auto draw(
        const SDL_GPUFactory& graphics_factory,
        CommandBuffer& command_buffer,
        const SDL_GPUInstanceBufferCache& instance_buffer_cache,
        gsl::span<const InstanceBatch> instance_batches,
        const Light& light_data,
        Utilities::PerformanceLogger& performance_logger
    ) -> void;

    [[nodiscard]] auto get_point_shadow_texture() const -> const Texture&;
    [[nodiscard]] auto get_point_shadow_sampler() const -> const Sampler&;
    [[nodiscard]] auto get_spot_shadow_texture() const -> const Texture&;
    [[nodiscard]] auto get_spot_shadow_sampler() const -> const Sampler&;
    [[nodiscard]] auto get_spot_shadow_matrix_buffer() const -> const Buffer&;

private:
    Shader shadow_vertex_shader;
    Shader shadow_fragment_shader;
    GraphicsPipeline shadow_pipeline;

    Texture point_shadow_texture;
    Sampler point_shadow_sampler;

    Texture spot_shadow_texture;
    Sampler spot_shadow_sampler;

    Buffer spot_shadow_matrix_buffer;
    TransferBuffer spot_shadow_matrix_transfer_buffer;
};

}  // namespace Luminol::Graphics::SDL_GPU
