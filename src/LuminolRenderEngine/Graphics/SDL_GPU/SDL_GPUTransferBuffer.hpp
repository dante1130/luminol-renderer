#pragma once

#include <cstdint>
#include <functional>
#include <memory>

#include <gsl/gsl>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTypes.hpp>

struct SDL_GPUTransferBuffer;

namespace Luminol::Graphics::SDL_GPU {

class GPUDevice;

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
        std::shared_ptr<GPUDevice> device,
        uint32_t size
    );

    [[nodiscard]] auto map(bool cycle) -> gsl::span<uint8_t>;
    auto unmap() -> void;

    [[nodiscard]] auto native_handle() const -> SDL_GPUTransferBuffer*;
    [[nodiscard]] auto get_size() const -> uint32_t;

private:
    std::unique_ptr<SDL_GPUTransferBuffer, SDL_GPUTransferBufferDeleter>
        transfer_buffer;
    std::shared_ptr<GPUDevice> device;
    uint32_t size;
};

}  // namespace Luminol::Graphics::SDL_GPU
