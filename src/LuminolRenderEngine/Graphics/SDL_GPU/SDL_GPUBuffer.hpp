#pragma once

#include <cstdint>
#include <functional>
#include <memory>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTypes.hpp>

struct SDL_GPUBuffer;

namespace Luminol::Graphics::SDL_GPU {

struct BufferInfo {
    BufferUsage usage;
    uint32_t size;
};

class Buffer {
public:
    using SDL_GPUBufferDeleter = std::function<void(SDL_GPUBuffer*)>;

    Buffer(
        std::unique_ptr<SDL_GPUBuffer, SDL_GPUBufferDeleter> buffer,
        uint32_t size
    );

    [[nodiscard]] auto native_handle() const -> SDL_GPUBuffer*;
    [[nodiscard]] auto get_size() const -> uint32_t;

private:
    std::unique_ptr<SDL_GPUBuffer, SDL_GPUBufferDeleter> buffer;
    uint32_t size;
};

}  // namespace Luminol::Graphics::SDL_GPU
