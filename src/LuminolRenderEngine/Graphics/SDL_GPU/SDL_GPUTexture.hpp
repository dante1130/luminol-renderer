#pragma once

struct SDL_GPUTexture;

namespace Luminol::Graphics::SDL_GPU {

// Non-owning view of an SDL_GPUTexture. Used to hand off swapchain textures
// (which SDL owns and reclaims each frame) to code that consumes them.
class TextureView {
public:
    explicit TextureView(SDL_GPUTexture* handle);

    [[nodiscard]] auto native_handle() const -> SDL_GPUTexture*;

private:
    SDL_GPUTexture* handle;
};

}  // namespace Luminol::Graphics::SDL_GPU
