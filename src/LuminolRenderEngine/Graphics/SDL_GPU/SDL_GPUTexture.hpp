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
    Texture2DArray,
    TextureCubeArray,
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
    // Explicit mip level count, used when generate_mipmaps is false (e.g. a
    // prefiltered specular cubemap where each level is rendered directly by
    // a dedicated pass rather than blitted). Callers requesting more than 1
    // level must include TextureUsage::ColorTarget themselves.
    uint32_t mip_levels = 1;
    // Number of array elements, only used when type is Texture2DArray or
    // TextureCubeArray (for TextureCubeArray, each element is 6 faces).
    uint32_t layer_count = 1;
};

class Texture {
public:
    using SDL_GPUTextureDeleter = std::function<void(SDL_GPUTexture*)>;

    // Constructed from a unique_ptr (matching GPUDevice::create_texture's
    // ownership handoff) but stored as a shared_ptr internally so Texture
    // itself is copyable - lets multiple material slots (e.g. metallic and
    // roughness, when they share one packed source image) bind the same GPU
    // texture without a duplicate upload.
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
    std::shared_ptr<SDL_GPUTexture> texture;
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
    // Highest mip level accessible when enable_mipmap_filtering is false
    // (mipmap_mode stays NEAREST, so an explicit SampleLevel/LOD picks one
    // mip exactly with no blending - needed for conservative Hi-Z reads).
    // Ignored when enable_mipmap_filtering is true (max_lod is unbounded).
    uint32_t max_lod = 0;
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
