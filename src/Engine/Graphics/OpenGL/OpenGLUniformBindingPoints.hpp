#pragma once

namespace Luminol::Graphics {

enum class UniformBufferBindingPoint {
    Transform = 0,
    Light,
    Material,
};

enum class SamplerBindingPoint {
    TextureDiffuse = 0,
};

}  // namespace Luminol::Graphics
