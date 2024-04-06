#pragma once

namespace Luminol::Graphics {

enum class UniformBufferBindingPoint {
    Transform = 0,
    Light,
};

enum class SamplerBindingPoint {
    TextureDiffuse = 0,
    TextureSpecular,
    TextureEmissive,
    TextureNormal,
    Skybox,
};

}  // namespace Luminol::Graphics
