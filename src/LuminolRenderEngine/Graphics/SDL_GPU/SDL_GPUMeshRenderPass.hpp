#pragma once

#include <unordered_map>
#include <vector>

#include <gsl/gsl>
#include <LuminolMaths/Matrix.hpp>

#include <LuminolRenderEngine/Graphics/RenderableManager.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUGraphicsPipeline.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUInstanceBufferCache.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUShader.hpp>

struct SDL_Window;

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

// Owns the mesh pipeline (position/uv vertex layout, view_proj uniform,
// per-instance model matrices via a storage buffer indexed by
// SV_InstanceID) and the persistent instance buffers backing it. Mirrors the
// per-pass class shape used by the OpenGL backend (e.g.
// OpenGLGBufferRenderPass): the pass owns its own GPU resources and exposes
// upload/draw steps, while SDL_GPURenderer just coordinates passes.
class SDL_GPUMeshRenderPass {
public:
    SDL_GPUMeshRenderPass(GPUDevice& device, SDL_Window* window);

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
        const Maths::Matrix4x4f& view_proj
    ) -> void;

private:
    Shader mesh_vertex_shader;
    Shader mesh_fragment_shader;
    GraphicsPipeline mesh_pipeline;

    SDL_GPUInstanceBufferCache instance_buffer_cache;
};

}  // namespace Luminol::Graphics::SDL_GPU
