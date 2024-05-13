#pragma once

#include <vector>

#include <glm/glm.hpp>

namespace Luminol::Graphics {

struct DirectionalLight {
    glm::vec3 direction = {0.0f, 0.0f, 0.0f};
    glm::vec3 color = {1.0f, 1.0f, 1.0f};
};

constexpr static auto max_point_lights = 256u;

struct PointLight {
    glm::vec3 position = {0.0f, 0.0f, 0.0f};
    glm::vec3 color = {1.0f, 1.0f, 1.0f};
};

constexpr static auto max_spot_lights = 256u;
constexpr static auto default_cut_off = 0.0f;
constexpr static auto default_outer_cut_off = 0.0f;

struct SpotLight {
    glm::vec3 position = {0.0f, 0.0f, 0.0f};
    glm::vec3 direction = {0.0f, 0.0f, 0.0f};
    glm::vec3 color = {1.0f, 1.0f, 1.0f};
    float cut_off = 0.0f;
    float outer_cut_off = 0.0f;
};

struct AlignedDirectionalLight {
    glm::vec4 direction = {glm::vec4{0.0f, 0.0f, 0.0f, 0.0f}};  // 16 bytes
    glm::vec4 color = {glm::vec4{1.0f, 1.0f, 1.0f, 1.0f}};      // 16 bytes
};

struct AlignedPointLight {
    glm::vec4 position = {glm::vec4{0.0f, 0.0f, 0.0f, 0.0f}};  // 16 bytes
    glm::vec4 color = {glm::vec4{1.0f, 1.0f, 1.0f, 1.0f}};     // 16 bytes
};

struct AlignedSpotLight {
    glm::vec4 position = {glm::vec4{0.0f, 0.0f, 0.0f, 0.0f}};   // 16 bytes
    glm::vec4 direction = {glm::vec4{0.0f, 0.0f, 0.0f, 0.0f}};  // 16 bytes
    glm::vec3 color = glm::vec3{1.0f, 1.0f, 1.0f};              // 12 bytes
    float cut_off = default_cut_off;                            // 4 bytes
    float outer_cut_off = default_outer_cut_off;                // 4 bytes
    glm::vec3 padding = glm::vec3{0.0f, 0.0f, 0.0f};            // 12 bytes
};

struct Light {
    AlignedDirectionalLight directional_light;  // 64 bytes
    uint32_t point_light_count = 0;             // 4 bytes
    uint32_t spot_light_count = 0;              // 4 bytes
    glm::vec2 padding = glm::vec2{0.0f, 0.0f};  // 8 bytes
    std::vector<AlignedPointLight> point_lights =
        std::vector<AlignedPointLight>{max_point_lights};  // 4096 bytes
    std::vector<AlignedSpotLight> spot_lights =
        std::vector<AlignedSpotLight>{max_spot_lights};  // 8192 bytes
};

constexpr static auto alignment = 16u;

static_assert(sizeof(AlignedDirectionalLight) % alignment == 0);
static_assert(sizeof(AlignedPointLight) % alignment == 0);
static_assert(sizeof(AlignedSpotLight) % alignment == 0);
static_assert(sizeof(Light) % alignment == 0);

}  // namespace Luminol::Graphics
