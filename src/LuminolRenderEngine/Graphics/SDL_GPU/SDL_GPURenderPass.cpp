#include "SDL_GPURenderPass.hpp"

#include <gsl/gsl>

#include <SDL3/SDL_gpu.h>

#include <vector>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUGraphicsPipeline.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTexture.hpp>

namespace {

using namespace Luminol::Graphics::SDL_GPU;

constexpr auto to_sdl_index_element_size(IndexElementSize size)
    -> SDL_GPUIndexElementSize {
    switch (size) {
        case IndexElementSize::Bits16:
            return SDL_GPU_INDEXELEMENTSIZE_16BIT;
        case IndexElementSize::Bits32:
            return SDL_GPU_INDEXELEMENTSIZE_32BIT;
    }
    throw std::runtime_error{"Invalid index element size"};
}

}  // namespace

namespace Luminol::Graphics::SDL_GPU {

RenderPass::RenderPass(SDL_GPURenderPass* render_pass)
    : render_pass{render_pass} {
    Expects(this->render_pass != nullptr);
}

RenderPass::~RenderPass() { end(); }

RenderPass::RenderPass(RenderPass&& other) noexcept
    : render_pass{other.render_pass} {
    other.render_pass = nullptr;
}

auto RenderPass::operator=(RenderPass&& other) noexcept -> RenderPass& {
    if (this != &other) {
        end();
        render_pass = other.render_pass;
        other.render_pass = nullptr;
    }
    return *this;
}

auto RenderPass::bind_graphics_pipeline(const GraphicsPipeline& pipeline)
    -> void {
    Expects(render_pass != nullptr);
    SDL_BindGPUGraphicsPipeline(render_pass, pipeline.native_handle());
}

auto RenderPass::set_viewport(
    float x,
    float y,
    float width,
    float height,
    float min_depth,
    float max_depth
) -> void {
    Expects(render_pass != nullptr);
    const auto viewport = SDL_GPUViewport{
        .x = x,
        .y = y,
        .w = width,
        .h = height,
        .min_depth = min_depth,
        .max_depth = max_depth,
    };
    SDL_SetGPUViewport(render_pass, &viewport);
}

auto RenderPass::set_scissor(int32_t x, int32_t y, int32_t width, int32_t height)
    -> void {
    Expects(render_pass != nullptr);
    const auto scissor = SDL_Rect{.x = x, .y = y, .w = width, .h = height};
    SDL_SetGPUScissor(render_pass, &scissor);
}

auto RenderPass::bind_vertex_buffers(
    uint32_t first_slot, gsl::span<const VertexBufferBinding> bindings
) -> void {
    Expects(render_pass != nullptr);

    auto sdl_bindings = std::vector<SDL_GPUBufferBinding>(bindings.size());
    for (auto i = size_t{0}; i < bindings.size(); ++i) {
        Expects(bindings[i].buffer != nullptr);
        sdl_bindings[i] = SDL_GPUBufferBinding{
            .buffer = bindings[i].buffer->native_handle(),
            .offset = bindings[i].offset,
        };
    }

    SDL_BindGPUVertexBuffers(
        render_pass,
        first_slot,
        sdl_bindings.data(),
        static_cast<uint32_t>(sdl_bindings.size())
    );
}

auto RenderPass::bind_vertex_storage_buffers(
    uint32_t first_slot, gsl::span<const Buffer* const> buffers
) -> void {
    Expects(render_pass != nullptr);

    auto sdl_buffers = std::vector<SDL_GPUBuffer*>(buffers.size());
    for (auto i = size_t{0}; i < buffers.size(); ++i) {
        Expects(buffers[i] != nullptr);
        sdl_buffers[i] = buffers[i]->native_handle();
    }

    SDL_BindGPUVertexStorageBuffers(
        render_pass,
        first_slot,
        sdl_buffers.data(),
        static_cast<uint32_t>(sdl_buffers.size())
    );
}

auto RenderPass::bind_fragment_samplers(
    uint32_t first_slot, gsl::span<const TextureSamplerBinding> bindings
) -> void {
    Expects(render_pass != nullptr);

    auto sdl_bindings =
        std::vector<SDL_GPUTextureSamplerBinding>(bindings.size());
    for (auto i = size_t{0}; i < bindings.size(); ++i) {
        Expects(bindings[i].texture != nullptr);
        Expects(bindings[i].sampler != nullptr);
        sdl_bindings[i] = SDL_GPUTextureSamplerBinding{
            .texture = bindings[i].texture->native_handle(),
            .sampler = bindings[i].sampler->native_handle(),
        };
    }

    SDL_BindGPUFragmentSamplers(
        render_pass,
        first_slot,
        sdl_bindings.data(),
        static_cast<uint32_t>(sdl_bindings.size())
    );
}

auto RenderPass::bind_fragment_storage_buffers(
    uint32_t first_slot, gsl::span<const Buffer* const> buffers
) -> void {
    Expects(render_pass != nullptr);

    auto sdl_buffers = std::vector<SDL_GPUBuffer*>(buffers.size());
    for (auto i = size_t{0}; i < buffers.size(); ++i) {
        Expects(buffers[i] != nullptr);
        sdl_buffers[i] = buffers[i]->native_handle();
    }

    SDL_BindGPUFragmentStorageBuffers(
        render_pass,
        first_slot,
        sdl_buffers.data(),
        static_cast<uint32_t>(sdl_buffers.size())
    );
}

auto RenderPass::draw_primitives(
    uint32_t num_vertices,
    uint32_t num_instances,
    uint32_t first_vertex,
    uint32_t first_instance
) -> void {
    Expects(render_pass != nullptr);
    SDL_DrawGPUPrimitives(
        render_pass,
        num_vertices,
        num_instances,
        first_vertex,
        first_instance
    );
}

auto RenderPass::bind_index_buffer(
    const Buffer& buffer, IndexElementSize element_size, uint32_t offset
) -> void {
    Expects(render_pass != nullptr);
    const auto binding = SDL_GPUBufferBinding{
        .buffer = buffer.native_handle(),
        .offset = offset,
    };
    SDL_BindGPUIndexBuffer(
        render_pass, &binding, to_sdl_index_element_size(element_size)
    );
}

auto RenderPass::draw_indexed_primitives(
    uint32_t num_indices,
    uint32_t num_instances,
    uint32_t first_index,
    int32_t vertex_offset,
    uint32_t first_instance
) -> void {
    Expects(render_pass != nullptr);
    SDL_DrawGPUIndexedPrimitives(
        render_pass,
        num_indices,
        num_instances,
        first_index,
        vertex_offset,
        first_instance
    );
}

auto RenderPass::draw_indexed_primitives_indirect(
    const Buffer& buffer, uint32_t offset, uint32_t draw_count
) -> void {
    Expects(render_pass != nullptr);
    SDL_DrawGPUIndexedPrimitivesIndirect(
        render_pass, buffer.native_handle(), offset, draw_count
    );
}

auto RenderPass::native_handle() const -> SDL_GPURenderPass* {
    return render_pass;
}

auto RenderPass::end() noexcept -> void {
    if (render_pass != nullptr) {
        SDL_EndGPURenderPass(render_pass);
        render_pass = nullptr;
    }
}

}  // namespace Luminol::Graphics::SDL_GPU
