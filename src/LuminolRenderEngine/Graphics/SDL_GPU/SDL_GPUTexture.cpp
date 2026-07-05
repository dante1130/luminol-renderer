#include "SDL_GPUTexture.hpp"

#include <gsl/gsl>

#include <SDL3/SDL_gpu.h>

namespace Luminol::Graphics::SDL_GPU {

Texture::Texture(SDL_GPUTexture* handle) : handle{handle} {
    Expects(this->handle != nullptr);
}

auto Texture::native_handle() const -> SDL_GPUTexture* { return handle; }

}  // namespace Luminol::Graphics::SDL_GPU
