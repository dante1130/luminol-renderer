#pragma once

#include <LuminolMaths/Matrix.hpp>

namespace Luminol::Graphics::OpenGLUniforms {

struct Transform {
    Maths::Matrix4x4f model_matrix = Maths::Matrix4x4f::identity();  // 64 bytes
    Maths::Matrix4x4f view_matrix = Maths::Matrix4x4f::identity();   // 64 bytes
    Maths::Matrix4x4f projection_matrix =
        Maths::Matrix4x4f::identity();  // 64 bytes
};

constexpr static auto alignment = 16u;

static_assert(sizeof(Transform) % alignment == 0);

}  // namespace Luminol::Graphics::OpenGLUniforms
