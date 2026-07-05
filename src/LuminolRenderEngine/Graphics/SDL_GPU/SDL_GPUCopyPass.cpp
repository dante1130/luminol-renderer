#include "SDL_GPUCopyPass.hpp"

#include <gsl/gsl>

#include <SDL3/SDL_gpu.h>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTransferBuffer.hpp>

namespace Luminol::Graphics::SDL_GPU {

CopyPass::CopyPass(SDL_GPUCopyPass* copy_pass) : copy_pass{copy_pass} {
    Expects(this->copy_pass != nullptr);
}

CopyPass::~CopyPass() { end(); }

CopyPass::CopyPass(CopyPass&& other) noexcept : copy_pass{other.copy_pass} {
    other.copy_pass = nullptr;
}

auto CopyPass::operator=(CopyPass&& other) noexcept -> CopyPass& {
    if (this != &other) {
        end();
        copy_pass = other.copy_pass;
        other.copy_pass = nullptr;
    }
    return *this;
}

auto CopyPass::upload_to_buffer(
    const TransferBuffer& source,
    uint32_t source_offset,
    const Buffer& destination,
    uint32_t destination_offset,
    uint32_t size,
    bool cycle
) -> void {
    Expects(copy_pass != nullptr);

    const auto source_location = SDL_GPUTransferBufferLocation{
        .transfer_buffer = source.native_handle(),
        .offset = source_offset,
    };

    const auto destination_region = SDL_GPUBufferRegion{
        .buffer = destination.native_handle(),
        .offset = destination_offset,
        .size = size,
    };

    SDL_UploadToGPUBuffer(
        copy_pass, &source_location, &destination_region, cycle
    );
}

auto CopyPass::native_handle() const -> SDL_GPUCopyPass* { return copy_pass; }

auto CopyPass::end() noexcept -> void {
    if (copy_pass != nullptr) {
        SDL_EndGPUCopyPass(copy_pass);
        copy_pass = nullptr;
    }
}

}  // namespace Luminol::Graphics::SDL_GPU
