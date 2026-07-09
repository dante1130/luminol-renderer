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

enum class TextureType : uint8_t {
    Texture2D,
    TextureCube,
};

struct TextureInfo {
    uint32_t width;
    uint32_t height;
    TextureFormat format = TextureFormat::R8G8B8A8_Unorm;
    TextureUsage usage = TextureUsage::Sampler;
    SampleCount sample_count = SampleCount::x1;
    TextureType type = TextureType::Texture2D;
    // When true, a full mip chain is allocated and the texture is given
    // SDL_GPU_TEXTUREUSAGE_COLOR_TARGET so SDL_GenerateMipmapsForGPUTexture
    // can blit into it.
    bool generate_mipmaps = false;
};

class Texture {
public:
    using SDL_GPUTextureDeleter = std::function<void(SDL_GPUTexture*)>;

    Texture(
        std::unique_ptr<SDL_GPUTexture, SDL_GPUTextureDeleter> texture,
        uint32_t width,
        uint32_t height,
        uint32_t mip_levels = 1
    );

    [[nodiscard]] auto native_handle() const -> SDL_GPUTexture*;
    [[nodiscard]] auto get_width() const -> uint32_t;
    [[nodiscard]] auto get_height() const -> uint32_t;
    [[nodiscard]] auto get_mip_levels() const -> uint32_t;

private:
    std::unique_ptr<SDL_GPUTexture, SDL_GPUTextureDeleter> texture;
    uint32_t width;
    uint32_t height;
    uint32_t mip_levels;
};

struct SamplerInfo {
    SamplerFilter filter = SamplerFilter::Linear;
    SamplerAddressMode address_mode_u = SamplerAddressMode::ClampToEdge;
    SamplerAddressMode address_mode_v = SamplerAddressMode::ClampToEdge;
    // Depth-comparison sampler (hardware PCF via SampleCmpLevelZero); always
    // uses a less-or-equal comparison, the only mode this codebase needs.
    bool enable_compare = false;
    // Enables trilinear mip filtering (mipmap_mode = LINEAR, max_lod
    // unbounded). Only meaningful for textures created with
    // TextureInfo::generate_mipmaps = true.
    bool enable_mipmap_filtering = false;
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
