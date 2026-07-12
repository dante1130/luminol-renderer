#pragma once

#include <array>
#include <unordered_map>
#include <vector>

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
// cascaded shadow map: the camera frustum is split into
// shadow_pass_num_cascades depth ranges, each fitted with its own
// tightly-bound orthographic box and rendered into its own layer of a
// Texture2DArray. This gives near geometry high texel density while still
// covering far view distances, unlike a single fixed-extent shadow map.
// Mirrors the per-pass class shape used by SDL_GPUAmbientOcclusionPass.
class SDL_GPUShadowPass {
public:
    explicit SDL_GPUShadowPass(GPUDevice& device);

    auto draw(
        const SDL_GPUFactory& graphics_factory,
        CommandBuffer& command_buffer,
        const SDL_GPUInstanceBufferCache& instance_buffer_cache,
        gsl::span<const InstanceBatch> instance_batches,
        const std::unordered_map<RenderableId, std::vector<Maths::Matrix4x4f>>&
            queued_draws,
        const Maths::Vector3f& light_direction,
        const Maths::Matrix4x4f& view_matrix,
        const Maths::Matrix4x4f& projection_matrix,
        Utilities::PerformanceLogger& performance_logger
    ) -> void;

    [[nodiscard]] auto get_shadow_map_texture() const -> const Texture&;
    [[nodiscard]] auto get_sampler() const -> const Sampler&;
    [[nodiscard]] auto get_cascade_light_space_matrices() const
        -> const std::array<Maths::Matrix4x4f, shadow_pass_num_cascades>&;
    [[nodiscard]] auto get_cascade_split_depths() const -> const Maths::Vector4f&;

private:
    Shader shadow_vertex_shader;
    Shader shadow_fragment_shader;
    GraphicsPipeline shadow_pipeline;

    Texture shadow_map_texture;
    Sampler shadow_map_sampler;

    std::array<Maths::Matrix4x4f, shadow_pass_num_cascades>
        cascade_light_space_matrices;
    Maths::Vector4f cascade_split_depths = Maths::Vector4f{0.0F, 0.0F, 0.0F, 0.0F};
};

}  // namespace Luminol::Graphics::SDL_GPU
