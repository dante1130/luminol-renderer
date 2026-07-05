#include "SDL_GPUTransferBuffer.hpp"

#include <gsl/gsl>

#include <SDL3/SDL_gpu.h>

namespace Luminol::Graphics::SDL_GPU {

TransferBuffer::TransferBuffer(
    std::unique_ptr<SDL_GPUTransferBuffer, SDL_GPUTransferBufferDeleter>
        transfer_buffer,
    SDL_GPUDevice* device,
    uint32_t size
)
    : transfer_buffer{std::move(transfer_buffer)},
      device{device},
      size{size} {
    Expects(this->transfer_buffer);
    Expects(this->device != nullptr);
}

auto TransferBuffer::map(bool cycle) -> gsl::span<uint8_t> {
    auto* mapped =
        SDL_MapGPUTransferBuffer(device, transfer_buffer.get(), cycle);
    return gsl::span<uint8_t>{static_cast<uint8_t*>(mapped), size};
}

auto TransferBuffer::unmap() -> void {
    SDL_UnmapGPUTransferBuffer(device, transfer_buffer.get());
}

auto TransferBuffer::native_handle() const -> SDL_GPUTransferBuffer* {
    return transfer_buffer.get();
}

auto TransferBuffer::get_size() const -> uint32_t { return size; }

}  // namespace Luminol::Graphics::SDL_GPU
