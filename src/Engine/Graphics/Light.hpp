#pragma once

#include <glm/glm.hpp>

namespace Luminol::Graphics {

struct DirectionalLight {
    glm::vec3 direction = {0.0f, 0.0f, 0.0f};
    glm::vec3 ambient = {1.0f, 1.0f, 1.0f};
    glm::vec3 diffuse = {1.0f, 1.0f, 1.0f};
    glm::vec3 specular = {1.0f, 1.0f, 1.0f};
};

constexpr static auto max_point_lights = 4;
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

}  // namespace Luminol::Graphics
