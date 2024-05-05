#pragma once

namespace Luminol::Graphics {

enum class BufferBit {
    Color = 0b001,
    Depth = 0b010,
    Stencil = 0b100,
    ColorDepth = Color | Depth,
    ColorStencil = Color | Stencil,
    DepthStencil = Depth | Stencil,
    ColorDepthStencil = Color | Depth | Stencil
};

}  // namespace Luminol::Graphics
