#pragma once

#include <cstdint>
#include <functional>
#include <memory>

#include <gsl/gsl>
#include <SDL3/SDL_gpu.h>

namespace Luminol::Graphics::SDL_GPU {

enum class TransferBufferUsage : uint32_t {
    Upload = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
    Download = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD,
};

struct TransferBufferInfo {
    TransferBufferUsage usage;
    uint32_t size;
};

class TransferBuffer {
public:
    using SDL_GPUTransferBufferDeleter =
        std::function<void(SDL_GPUTransferBuffer*)>;

    TransferBuffer(
        std::unique_ptr<SDL_GPUTransferBuffer, SDL_GPUTransferBufferDeleter>
            transfer_buffer,
        SDL_GPUDevice* device,
        uint32_t size
    );

    [[nodiscard]] auto map(bool cycle) -> gsl::span<uint8_t>;
    auto unmap() -> void;

    [[nodiscard]] auto get() const -> SDL_GPUTransferBuffer*;
    [[nodiscard]] auto get_size() const -> uint32_t;

private:
    std::unique_ptr<SDL_GPUTransferBuffer, SDL_GPUTransferBufferDeleter>
        transfer_buffer;
    SDL_GPUDevice* device;
    uint32_t size;
};

}  // namespace Luminol::Graphics::SDL_GPU
