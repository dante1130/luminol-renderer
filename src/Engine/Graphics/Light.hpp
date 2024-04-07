#pragma once

#include <array>

#include <glm/glm.hpp>

namespace Luminol::Graphics {

struct DirectionalLight {
    glm::vec3 direction = {0.0f, 0.0f, 0.0f};
    glm::vec3 ambient = {1.0f, 1.0f, 1.0f};
    glm::vec3 diffuse = {1.0f, 1.0f, 1.0f};
    glm::vec3 specular = {1.0f, 1.0f, 1.0f};
};

constexpr static auto max_point_lights = 4u;
constexpr static auto default_constant = 1.0f;
constexpr static auto default_linear = 0.09f;
constexpr static auto default_quadratic = 0.032f;

struct PointLight {
    glm::vec3 position = {0.0f, 0.0f, 0.0f};
    glm::vec3 ambient = {1.0f, 1.0f, 1.0f};
    glm::vec3 diffuse = {1.0f, 1.0f, 1.0f};
    glm::vec3 specular = {1.0f, 1.0f, 1.0f};
    float constant = default_constant;
    float linear = default_linear;
    float quadratic = default_quadratic;
};

constexpr static auto max_spot_lights = 4u;
constexpr static auto default_cut_off = 0.0f;
constexpr static auto default_outer_cut_off = 0.0f;

struct SpotLight {
    glm::vec3 position = {0.0f, 0.0f, 0.0f};
    glm::vec3 direction = {0.0f, 0.0f, 0.0f};
    glm::vec3 ambient = {1.0f, 1.0f, 1.0f};
    glm::vec3 diffuse = {1.0f, 1.0f, 1.0f};
    glm::vec3 specular = {1.0f, 1.0f, 1.0f};
    float constant = default_constant;
    float linear = default_linear;
    float quadratic = default_quadratic;
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
