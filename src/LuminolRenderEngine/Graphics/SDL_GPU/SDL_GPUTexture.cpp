#include "SDL_GPUTexture.hpp"

#include <gsl/gsl>

#include <SDL3/SDL_gpu.h>

namespace Luminol::Graphics::SDL_GPU {

TextureView::TextureView(SDL_GPUTexture* handle) : handle{handle} {
    Expects(this->handle != nullptr);
}

auto TextureView::native_handle() const -> SDL_GPUTexture* { return handle; }

Texture::Texture(
    std::unique_ptr<SDL_GPUTexture, SDL_GPUTextureDeleter> texture,
    uint32_t width,
    uint32_t height
)
    : texture{std::move(texture)}, width{width}, height{height} {
    Expects(this->texture != nullptr);
}

auto Texture::native_handle() const -> SDL_GPUTexture* {
    return texture.get();
}

auto Texture::get_width() const -> uint32_t { return width; }

auto Texture::get_height() const -> uint32_t { return height; }

Sampler::Sampler(std::unique_ptr<SDL_GPUSampler, SDL_GPUSamplerDeleter> sampler
)
    : sampler{std::move(sampler)} {
    Expects(this->sampler != nullptr);
}

auto Sampler::native_handle() const -> SDL_GPUSampler* {
    return sampler.get();
}

}  // namespace Luminol::Graphics::SDL_GPU
