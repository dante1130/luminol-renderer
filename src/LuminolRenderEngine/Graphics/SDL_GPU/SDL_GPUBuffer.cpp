#include "SDL_GPUBuffer.hpp"

#include <gsl/gsl>

namespace Luminol::Graphics::SDL_GPU {

Buffer::Buffer(
    std::unique_ptr<SDL_GPUBuffer, SDL_GPUBufferDeleter> buffer, uint32_t size
)
    : buffer{std::move(buffer)}, size{size} {
    Expects(this->buffer);
}

auto Buffer::get() const -> SDL_GPUBuffer* { return buffer.get(); }

auto Buffer::get_size() const -> uint32_t { return size; }

}  // namespace Luminol::Graphics::SDL_GPU
