#pragma once

#include <cstdint>

#include <gsl/gsl>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTypes.hpp>

struct SDL_GPURenderPass;

namespace Luminol::Graphics::SDL_GPU {

class GraphicsPipeline;

class RenderPass {
public:
    RenderPass(SDL_GPURenderPass* render_pass);
    ~RenderPass();

    RenderPass(const RenderPass&) = delete;
    auto operator=(const RenderPass&) -> RenderPass& = delete;

    RenderPass(RenderPass&& other) noexcept;
    auto operator=(RenderPass&& other) noexcept -> RenderPass&;

    auto bind_graphics_pipeline(const GraphicsPipeline& pipeline) -> void;

    auto bind_vertex_buffers(
        uint32_t first_slot, gsl::span<const VertexBufferBinding> bindings
    ) -> void;

    auto bind_fragment_samplers(
        uint32_t first_slot, gsl::span<const TextureSamplerBinding> bindings
    ) -> void;

    auto bind_index_buffer(
        const Buffer& buffer,
        IndexElementSize element_size,
        uint32_t offset = 0
    ) -> void;

    auto draw_indexed_primitives(
        uint32_t num_indices,
        uint32_t num_instances = 1,
        uint32_t first_index = 0,
        int32_t vertex_offset = 0,
        uint32_t first_instance = 0
    ) -> void;

    auto draw_primitives(
        uint32_t num_vertices,
        uint32_t num_instances = 1,
        uint32_t first_vertex = 0,
        uint32_t first_instance = 0
    ) -> void;

    [[nodiscard]] auto native_handle() const -> SDL_GPURenderPass*;

private:
    auto end() noexcept -> void;

    SDL_GPURenderPass* render_pass;
};

}  // namespace Luminol::Graphics::SDL_GPU
