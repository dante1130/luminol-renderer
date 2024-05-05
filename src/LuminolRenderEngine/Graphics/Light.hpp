#pragma once

#include <array>

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

struct Light {
    DirectionalLight directional_light;
    std::array<PointLight, max_point_lights> point_lights;
    std::array<SpotLight, max_spot_lights> spot_lights;
    uint32_t point_light_count = 0u;
    uint32_t spot_light_count = 0u;
};

}  // namespace Luminol::Graphics
