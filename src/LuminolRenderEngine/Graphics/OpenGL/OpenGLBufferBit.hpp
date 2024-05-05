#pragma once

#include <LuminolRenderEngine/Graphics/BufferBit.hpp>

using GLenum = unsigned int;

namespace Luminol::Graphics {

auto buffer_bit_to_gl(BufferBit buffer_bit) -> GLenum;

}  // namespace Luminol::Graphics
