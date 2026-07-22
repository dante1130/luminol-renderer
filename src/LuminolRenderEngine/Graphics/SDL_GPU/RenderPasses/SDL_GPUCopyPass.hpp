#pragma once

#include <cstdint>

struct SDL_GPUCopyPass;

namespace Luminol::Graphics::SDL_GPU {

class Buffer;
class TransferBuffer;
class Texture;

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

    auto download_from_buffer(
        const Buffer& source,
        uint32_t source_offset,
        const TransferBuffer& destination,
        uint32_t destination_offset,
        uint32_t size
    ) -> void;

    auto upload_to_texture(
        const TransferBuffer& source,
        uint32_t source_offset,
        const Texture& destination,
        uint32_t width,
        uint32_t height,
        bool cycle,
        uint32_t layer = 0
    ) -> void;

    auto download_from_texture(
        const Texture& source,
        uint32_t mip_level,
        uint32_t width,
        uint32_t height,
        const TransferBuffer& destination,
        uint32_t destination_offset,
        uint32_t layer = 0
    ) -> void;

    [[nodiscard]] auto native_handle() const -> SDL_GPUCopyPass*;

private:
    auto end() noexcept -> void;

    SDL_GPUCopyPass* copy_pass;
};

}  // namespace Luminol::Graphics::SDL_GPU
