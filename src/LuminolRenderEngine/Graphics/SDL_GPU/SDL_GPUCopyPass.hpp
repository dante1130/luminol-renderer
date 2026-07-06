#pragma once

#include <cstdint>

struct SDL_GPUCopyPass;

namespace Luminol::Graphics::SDL_GPU {

class Buffer;
class TransferBuffer;

class CopyPass {
public:
    CopyPass(SDL_GPUCopyPass* copy_pass);
    ~CopyPass();

    CopyPass(const CopyPass&) = delete;
    auto operator=(const CopyPass&) -> CopyPass& = delete;

    CopyPass(CopyPass&& other) noexcept;
    auto operator=(CopyPass&& other) noexcept -> CopyPass&;

    auto upload_to_buffer(
        const TransferBuffer& source,
        uint32_t source_offset,
        const Buffer& destination,
        uint32_t destination_offset,
        uint32_t size,
        bool cycle
    ) -> void;

    [[nodiscard]] auto native_handle() const -> SDL_GPUCopyPass*;

private:
    auto end() noexcept -> void;

    SDL_GPUCopyPass* copy_pass;
};

}  // namespace Luminol::Graphics::SDL_GPU
