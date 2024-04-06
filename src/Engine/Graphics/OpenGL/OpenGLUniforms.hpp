#pragma once

#include <array>

#include <glm/glm.hpp>

#include <Engine/Graphics/Light.hpp>

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

struct DirectionalLight {
    PaddedVec3 direction = {glm::vec3{0.0f, 0.0f, 0.0f}};  // 16 bytes
    PaddedVec3 ambient = {glm::vec3{1.0f, 1.0f, 1.0f}};    // 16 bytes
    PaddedVec3 diffuse = {glm::vec3{1.0f, 1.0f, 1.0f}};    // 16 bytes
    PaddedVec3 specular = {glm::vec3{1.0f, 1.0f, 1.0f}};   // 16 bytes
};

struct PointLight {
    PaddedVec3 position = {glm::vec3{0.0f, 0.0f, 0.0f}};  // 16 bytes
    PaddedVec3 ambient = {glm::vec3{1.0f, 1.0f, 1.0f}};   // 16 bytes
    PaddedVec3 diffuse = {glm::vec3{1.0f, 1.0f, 1.0f}};   // 16 bytes
    PaddedVec3 specular = {glm::vec3{1.0f, 1.0f, 1.0f}};  // 16 bytes
    float constant = default_constant;                    // 4 bytes
    float linear = default_linear;                        // 4 bytes
    float quadratic = default_quadratic;                  // 4 bytes
};

struct Light {
    DirectionalLight directional_light;                     // 64 bytes
    std::array<PointLight, max_point_lights> point_lights;  // 512 bytes
    uint32_t point_light_count = 0;                         // 4 bytes
};

}  // namespace OpenGLUniforms

}  // namespace Luminol::Graphics
