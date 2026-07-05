#pragma once

struct SDL_GPUTexture;

namespace Luminol::Graphics::SDL_GPU {

class Texture {
public:
    explicit Texture(SDL_GPUTexture* handle);

    [[nodiscard]] auto native_handle() const -> SDL_GPUTexture*;

private:
    SDL_GPUTexture* handle;
};

}  // namespace Luminol::Graphics::SDL_GPU
