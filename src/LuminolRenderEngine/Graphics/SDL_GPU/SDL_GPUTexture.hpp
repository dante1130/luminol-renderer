#pragma once

#include <cstdint>
#include <functional>
#include <memory>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTypes.hpp>

struct SDL_GPUTexture;
struct SDL_GPUSampler;

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

struct TextureInfo {
    uint32_t width;
    uint32_t height;
    TextureFormat format = TextureFormat::R8G8B8A8_Unorm;
    TextureUsage usage = TextureUsage::Sampler;
};

class Texture {
public:
    using SDL_GPUTextureDeleter = std::function<void(SDL_GPUTexture*)>;

    Texture(
        std::unique_ptr<SDL_GPUTexture, SDL_GPUTextureDeleter> texture,
        uint32_t width,
        uint32_t height
    );

    [[nodiscard]] auto native_handle() const -> SDL_GPUTexture*;
    [[nodiscard]] auto get_width() const -> uint32_t;
    [[nodiscard]] auto get_height() const -> uint32_t;

private:
    std::unique_ptr<SDL_GPUTexture, SDL_GPUTextureDeleter> texture;
    uint32_t width;
    uint32_t height;
};

struct SamplerInfo {
    SamplerFilter filter = SamplerFilter::Linear;
    SamplerAddressMode address_mode = SamplerAddressMode::ClampToEdge;
    // Depth-comparison sampler (hardware PCF via SampleCmpLevelZero); always
    // uses a less-or-equal comparison, the only mode this codebase needs.
    bool enable_compare = false;
};

class Sampler {
public:
    using SDL_GPUSamplerDeleter = std::function<void(SDL_GPUSampler*)>;

    Sampler(std::unique_ptr<SDL_GPUSampler, SDL_GPUSamplerDeleter> sampler);

    [[nodiscard]] auto native_handle() const -> SDL_GPUSampler*;

private:
    std::unique_ptr<SDL_GPUSampler, SDL_GPUSamplerDeleter> sampler;
};

}  // namespace Luminol::Graphics::SDL_GPU
