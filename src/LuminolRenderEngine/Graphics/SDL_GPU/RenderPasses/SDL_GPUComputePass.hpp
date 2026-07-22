#pragma once

#include <cstdint>

#include <gsl/gsl>

struct SDL_GPUComputePass;

namespace Luminol::Graphics::SDL_GPU {

class Buffer;
class ComputePipeline;
class Texture;
struct TextureSamplerBinding;

class ComputePass {
public:
    ComputePass(SDL_GPUComputePass* compute_pass);
    ~ComputePass();

    ComputePass(const ComputePass&) = delete;
    auto operator=(const ComputePass&) -> ComputePass& = delete;

    ComputePass(ComputePass&& other) noexcept;
    auto operator=(ComputePass&& other) noexcept -> ComputePass&;

    auto bind_compute_pipeline(const ComputePipeline& pipeline) -> void;

    // Read-only storage buffers (BufferUsage::ComputeStorageRead).
    auto bind_storage_buffers(
        uint32_t first_slot, gsl::span<const Buffer* const> buffers
    ) -> void;

    // Read-only storage textures (TextureUsage::ComputeStorageRead).
    auto bind_storage_textures(
        uint32_t first_slot, gsl::span<const Texture* const> textures
    ) -> void;

    // Sampled textures (TextureUsage::Sampler) for compute shaders that
    // sample with an arbitrary/computed LOD (e.g. SampleLevel).
    auto bind_samplers(
        uint32_t first_slot, gsl::span<const TextureSamplerBinding> bindings
    ) -> void;

    auto dispatch(
        uint32_t groupcount_x, uint32_t groupcount_y, uint32_t groupcount_z
    ) -> void;

    [[nodiscard]] auto native_handle() const -> SDL_GPUComputePass*;

private:
    auto end() noexcept -> void;

    SDL_GPUComputePass* compute_pass;
};

}  // namespace Luminol::Graphics::SDL_GPU
