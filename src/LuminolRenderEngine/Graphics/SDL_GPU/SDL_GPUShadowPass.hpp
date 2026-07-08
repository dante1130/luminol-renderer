#pragma once

#include <gsl/gsl>
#include <LuminolMaths/Matrix.hpp>
#include <LuminolMaths/Vector.hpp>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUGraphicsPipeline.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUInstanceBufferCache.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUMeshRenderPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUShader.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTexture.hpp>
#include <LuminolRenderEngine/Utilities/PerformanceLogger.hpp>

namespace Luminol::Graphics::SDL_GPU {

class GPUDevice;
class CommandBuffer;
class SDL_GPUFactory;

// Renders scene depth from the directional light's point of view into a
// fixed-resolution shadow map. The light frustum is an orthographic box that
// follows the camera (not scene-fitted, no cascades): simple and stable, at
// the cost of a fixed shadow distance/extent. Mirrors the per-pass class
// shape used by SDL_GPUAmbientOcclusionPass.
class SDL_GPUShadowPass {
public:
    explicit SDL_GPUShadowPass(GPUDevice& device);

    auto draw(
        const SDL_GPUFactory& graphics_factory,
        CommandBuffer& command_buffer,
        const SDL_GPUInstanceBufferCache& instance_buffer_cache,
        gsl::span<const InstanceBatch> instance_batches,
        const Maths::Vector3f& light_direction,
        const Maths::Vector3f& camera_position,
        Utilities::PerformanceLogger& performance_logger
    ) -> void;

    [[nodiscard]] auto get_shadow_map_texture() const -> const Texture&;
    [[nodiscard]] auto get_sampler() const -> const Sampler&;
    [[nodiscard]] auto get_light_space_matrix() const -> const Maths::Matrix4x4f&;

private:
    Shader shadow_vertex_shader;
    Shader shadow_fragment_shader;
    GraphicsPipeline shadow_pipeline;

    Texture shadow_map_texture;
    Sampler shadow_map_sampler;

    Maths::Matrix4x4f light_space_matrix = Maths::Matrix4x4f::identity();
};

}  // namespace Luminol::Graphics::SDL_GPU
