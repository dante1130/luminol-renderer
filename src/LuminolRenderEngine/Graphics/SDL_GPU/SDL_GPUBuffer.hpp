#pragma once

#include <cstdint>
#include <functional>
#include <memory>

#include <SDL3/SDL_gpu.h>

namespace Luminol::Graphics::SDL_GPU {

enum class BufferUsage : uint32_t {
    Vertex = SDL_GPU_BUFFERUSAGE_VERTEX,
    Index = SDL_GPU_BUFFERUSAGE_INDEX,
};

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

    [[nodiscard]] auto get() const -> SDL_GPUBuffer*;
    [[nodiscard]] auto get_size() const -> uint32_t;

private:
    std::unique_ptr<SDL_GPUBuffer, SDL_GPUBufferDeleter> buffer;
    uint32_t size;
};

}  // namespace Luminol::Graphics::SDL_GPU
