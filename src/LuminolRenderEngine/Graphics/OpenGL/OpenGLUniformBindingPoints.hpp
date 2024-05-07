#pragma once

namespace Luminol::Graphics {

enum class UniformBufferBindingPoint {
    Transform = 0,
    Light,
};

enum class ShaderStorageBufferBindingPoint {
    InstancingModelMatrices = 0,
    Color,
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

}  // namespace Luminol::Graphics
