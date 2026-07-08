#pragma once

#include <unordered_map>
#include <vector>

#include <gsl/gsl>
#include <LuminolMaths/Matrix.hpp>
#include <LuminolMaths/Vector.hpp>

#include <LuminolRenderEngine/Graphics/RenderableManager.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUGraphicsPipeline.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUInstanceBufferCache.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUShader.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTexture.hpp>

namespace Luminol::Graphics::SDL_GPU {

class GPUDevice;
class CopyPass;
class CommandBuffer;
class RenderPass;
class SDL_GPUFactory;

struct InstanceBatch {
    RenderableId renderable_id;
    uint32_t instance_count;
};

// Layout matches the LightBuffer cbuffer in pbr_frag.hlsl (register b0,
// space3): four float4s, so no manual padding is needed.
struct DirectionalLightData {
    Maths::Vector4f direction;
    Maths::Vector4f color;
    Maths::Vector4f view_position;
    Maths::Vector4f screen_size;
};

// Owns the mesh pipeline (position/uv vertex layout, view_proj uniform,
// per-instance model matrices via a storage buffer indexed by
// SV_InstanceID) and the persistent instance buffers backing it. Mirrors the
// per-pass class shape used by the OpenGL backend (e.g.
// OpenGLGBufferRenderPass): the pass owns its own GPU resources and exposes
// upload/draw steps, while SDL_GPURenderer just coordinates passes.
class SDL_GPUMeshRenderPass {
public:
    explicit SDL_GPUMeshRenderPass(GPUDevice& device);

    [[nodiscard]] auto upload_instances(
        GPUDevice& device,
        CopyPass& copy_pass,
        const std::unordered_map<RenderableId, std::vector<Maths::Matrix4x4f>>&
            queued_draws
    ) -> std::vector<InstanceBatch>;

    auto draw(
        const SDL_GPUFactory& graphics_factory,
        CommandBuffer& command_buffer,
        RenderPass& render_pass,
        gsl::span<const InstanceBatch> instance_batches,
        const Maths::Matrix4x4f& view_proj,
        const DirectionalLightData& light_data,
        const Texture& ssao_texture,
        const Sampler& ssao_sampler
    ) -> void;

    [[nodiscard]] auto get_instance_buffer_cache() const
        -> const SDL_GPUInstanceBufferCache&;

private:
    Shader mesh_vertex_shader;
    Shader mesh_fragment_shader;
    GraphicsPipeline mesh_pipeline;

    SDL_GPUInstanceBufferCache instance_buffer_cache;
};

}  // namespace Luminol::Graphics::SDL_GPU
