#pragma once

#include <array>

#include <glm/glm.hpp>

#include <LuminolRenderEngine/Graphics/Light.hpp>

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
    PaddedVec3 color = {glm::vec3{1.0f, 1.0f, 1.0f}};      // 16 bytes
};

struct PointLight {
    PaddedVec3 position = {glm::vec3{0.0f, 0.0f, 0.0f}};  // 16 bytes
    PaddedVec3 color = {glm::vec3{1.0f, 1.0f, 1.0f}};     // 16 bytes
};

struct SpotLight {
    PaddedVec3 position = {glm::vec3{0.0f, 0.0f, 0.0f}};   // 16 bytes
    PaddedVec3 direction = {glm::vec3{0.0f, 0.0f, 0.0f}};  // 16 bytes
    glm::vec3 color = glm::vec3{1.0f, 1.0f, 1.0f};         // 12 bytes
    float cut_off = default_cut_off;                       // 4 bytes
    float outer_cut_off = default_outer_cut_off;           // 4 bytes
    glm::vec3 padding = glm::vec3{0.0f, 0.0f, 0.0f};       // 12 bytes
};

struct Light {
    DirectionalLight directional_light;                     // 64 bytes
    uint32_t point_light_count = 0;                         // 4 bytes
    uint32_t spot_light_count = 0;                          // 4 bytes
    glm::vec2 padding = glm::vec2{0.0f, 0.0f};              // 8 bytes
    std::array<PointLight, max_point_lights> point_lights;  // 320 bytes
    std::array<SpotLight, max_spot_lights> spot_lights;     // 384 bytes
};

constexpr static auto alignment = 16u;

static_assert(sizeof(Transform) % alignment == 0);
static_assert(sizeof(DirectionalLight) % alignment == 0);
static_assert(sizeof(PointLight) % alignment == 0);
static_assert(sizeof(SpotLight) % alignment == 0);

}  // namespace OpenGLUniforms

}  // namespace Luminol::Graphics
