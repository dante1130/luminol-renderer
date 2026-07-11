#include "SDL_GPUComputePass.hpp"

#include <gsl/gsl>

#include <SDL3/SDL_gpu.h>

#include <vector>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUComputePipeline.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTexture.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTypes.hpp>

namespace Luminol::Graphics::SDL_GPU {

ComputePass::ComputePass(SDL_GPUComputePass* compute_pass)
    : compute_pass{compute_pass} {
    Expects(this->compute_pass != nullptr);
}

ComputePass::~ComputePass() { end(); }

ComputePass::ComputePass(ComputePass&& other) noexcept
    : compute_pass{other.compute_pass} {
    other.compute_pass = nullptr;
}

auto ComputePass::operator=(ComputePass&& other) noexcept -> ComputePass& {
    if (this != &other) {
        end();
        compute_pass = other.compute_pass;
        other.compute_pass = nullptr;
    }
    return *this;
}

auto ComputePass::bind_compute_pipeline(const ComputePipeline& pipeline)
    -> void {
    Expects(compute_pass != nullptr);
    SDL_BindGPUComputePipeline(compute_pass, pipeline.native_handle());
}

auto ComputePass::bind_storage_buffers(
    uint32_t first_slot, gsl::span<const Buffer* const> buffers
) -> void {
    Expects(compute_pass != nullptr);

    auto sdl_buffers = std::vector<SDL_GPUBuffer*>(buffers.size());
    for (auto i = size_t{0}; i < buffers.size(); ++i) {
        Expects(buffers[i] != nullptr);
        sdl_buffers[i] = buffers[i]->native_handle();
    }

    SDL_BindGPUComputeStorageBuffers(
        compute_pass,
        first_slot,
        sdl_buffers.data(),
        static_cast<uint32_t>(sdl_buffers.size())
    );
}

auto ComputePass::bind_storage_textures(
    uint32_t first_slot, gsl::span<const Texture* const> textures
) -> void {
    Expects(compute_pass != nullptr);

    auto sdl_textures = std::vector<SDL_GPUTexture*>(textures.size());
    for (auto i = size_t{0}; i < textures.size(); ++i) {
        Expects(textures[i] != nullptr);
        sdl_textures[i] = textures[i]->native_handle();
    }

    SDL_BindGPUComputeStorageTextures(
        compute_pass,
        first_slot,
        sdl_textures.data(),
        static_cast<uint32_t>(sdl_textures.size())
    );
}

auto ComputePass::bind_samplers(
    uint32_t first_slot, gsl::span<const TextureSamplerBinding> bindings
) -> void {
    Expects(compute_pass != nullptr);

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

    SDL_BindGPUComputeSamplers(
        compute_pass,
        first_slot,
        sdl_bindings.data(),
        static_cast<uint32_t>(sdl_bindings.size())
    );
}

auto ComputePass::dispatch(
    uint32_t groupcount_x, uint32_t groupcount_y, uint32_t groupcount_z
) -> void {
    Expects(compute_pass != nullptr);
    SDL_DispatchGPUCompute(
        compute_pass, groupcount_x, groupcount_y, groupcount_z
    );
}

auto ComputePass::native_handle() const -> SDL_GPUComputePass* {
    return compute_pass;
}

auto ComputePass::end() noexcept -> void {
    if (compute_pass != nullptr) {
        SDL_EndGPUComputePass(compute_pass);
        compute_pass = nullptr;
    }
}

}  // namespace Luminol::Graphics::SDL_GPU
