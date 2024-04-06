#pragma once

#include <glm/glm.hpp>

namespace Luminol::Graphics {

struct Light {
    glm::vec3 position = {0.0f, 0.0f, 0.0f};
    glm::vec3 ambient = {1.0f, 1.0f, 1.0f};
    glm::vec3 diffuse = {1.0f, 1.0f, 1.0f};
    glm::vec3 specular = {1.0f, 1.0f, 1.0f};
};

}  // namespace Luminol::Graphics
