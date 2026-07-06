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
