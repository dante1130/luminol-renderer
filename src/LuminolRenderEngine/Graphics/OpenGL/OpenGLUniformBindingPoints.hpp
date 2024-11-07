#pragma once

#include <cstdint>

namespace Luminol::Graphics {

enum class UniformBufferBindingPoint : uint8_t {
    Transform = 0,
    Light,
};

enum class ShaderStorageBufferBindingPoint : uint8_t {
    InstancingModelMatrices = 0,
    Color,
    LuminanceHistogram,
};

enum class SamplerBindingPoint : uint8_t {
    TextureDiffuse = 0,
    TextureEmissive,
    TextureNormal,
    TextureMetallic,
    TextureRoughness,
    TextureAO,
    Skybox,
    HDRFramebuffer,
    GBufferPositionMetallic,
    GBufferNormalRoughness,
    GBufferEmissiveAO,
    GBufferAlbedo,
};

enum class ImageBindingPoint : uint8_t {
    Screen = 0,
    HDRFramebuffer,
    AverageLuminance,
};

}  // namespace Luminol::Graphics
