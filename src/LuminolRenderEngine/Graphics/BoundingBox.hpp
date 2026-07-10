#pragma once

#include <LuminolMaths/Vector.hpp>

namespace Luminol::Graphics {

struct BoundingBox {
    Maths::Vector3f min;
    Maths::Vector3f max;
};

}  // namespace Luminol::Graphics
