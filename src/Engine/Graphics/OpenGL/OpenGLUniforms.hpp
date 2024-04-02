#pragma once

#include <glm/glm.hpp>

namespace Luminol::Graphics {

struct PaddedVec3 {
    glm::vec3 data = {0.0f, 0.0f, 0.0f};  // 12 bytes
    float padding = 0.0f;                 // 4 bytes
};

namespace OpenGLUniforms {

struct Transform {
    glm::mat4 model_matrix = {1.0f};       // 64 bytes
    glm::mat4 view_matrix = {1.0f};        // 64 bytes
    glm::mat4 projection_matrix = {1.0f};  // 64 bytes
};

struct Light {
    PaddedVec3 position = {.data = {0.0f, 0.0f, 0.0f}};  // 16 bytes
    glm::vec3 color = {1.0f, 1.0f, 1.0f};                // 12 bytes
    float ambient_intensity = {1.0f};                    // 4 bytes
};

}  // namespace OpenGLUniforms

}  // namespace Luminol::Graphics
