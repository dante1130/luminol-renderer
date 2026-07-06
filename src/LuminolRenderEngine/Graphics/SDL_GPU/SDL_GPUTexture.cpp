#include "SDL_GPUTexture.hpp"

#include <gsl/gsl>

#include <SDL3/SDL_gpu.h>

namespace Luminol::Graphics::SDL_GPU {

TextureView::TextureView(SDL_GPUTexture* handle) : handle{handle} {
    Expects(this->handle != nullptr);
}

auto TextureView::native_handle() const -> SDL_GPUTexture* { return handle; }

}  // namespace Luminol::Graphics::SDL_GPU
