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
    PaddedVec3 position = {glm::vec3{0.0f, 0.0f, 0.0f}};  // 16 bytes
    glm::vec3 color = {glm::vec3{1.0f, 1.0f, 1.0f}};      // 12 bytes
    float ambient_intensity = {1.0f};                     // 4 bytes
    float specular_intensity = {1.0f};                    // 4 bytes
};

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
struct Material {
    PaddedVec3 ambient = {glm::vec3{1.0f, 1.0f, 1.0f}};  // 16 bytes
    PaddedVec3 diffuse = {glm::vec3{1.0f, 1.0f, 1.0f}};  // 16 bytes
    glm::vec3 specular = {glm::vec3{1.0f, 1.0f, 1.0f}};  // 12 bytes
    float shininess = 32.0f;                             // 4 bytes
};
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)

}  // namespace OpenGLUniforms

}  // namespace Luminol::Graphics
