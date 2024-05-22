#pragma once

namespace Luminol::Graphics {

enum class UniformBufferBindingPoint {
    Transform = 0,
    Light,
};

enum class ShaderStorageBufferBindingPoint {
    InstancingModelMatrices = 0,
    Color,
    LuminanceHistogram,
};

enum class SamplerBindingPoint {
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

enum class ImageBindingPoint {
    Screen = 0,
    HDRFramebuffer,
    AverageLuminance,
};

}  // namespace Luminol::Graphics
