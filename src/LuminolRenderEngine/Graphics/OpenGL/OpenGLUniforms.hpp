#pragma once

#include <glm/glm.hpp>

namespace Luminol::Graphics::OpenGLUniforms {

struct Transform {
    glm::mat4 model_matrix = {1.0f};       // 64 bytes
    glm::mat4 view_matrix = {1.0f};        // 64 bytes
    glm::mat4 projection_matrix = {1.0f};  // 64 bytes
};

constexpr static auto alignment = 16u;

static_assert(sizeof(Transform) % alignment == 0);

}  // namespace Luminol::Graphics::OpenGLUniforms
