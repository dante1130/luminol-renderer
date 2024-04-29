#pragma once

namespace Luminol::Graphics {

enum class UniformBufferBindingPoint {
    Transform = 0,
    Light,
};

enum class SamplerBindingPoint {
    TextureDiffuse = 0,
    TextureEmissive,
    TextureNormal,
    Skybox,
    HDRFramebuffer,
    GBufferPosition,
    GBufferNormal,
    GBufferEmissive,
    GBufferAlbedo,
};

}  // namespace Luminol::Graphics
