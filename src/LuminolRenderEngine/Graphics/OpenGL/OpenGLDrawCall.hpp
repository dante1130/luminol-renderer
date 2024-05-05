#pragma once

#include <glm/glm.hpp>

#include <LuminolRenderEngine/Graphics/Renderable.hpp>

namespace Luminol::Graphics {

struct DrawCall {
    std::reference_wrapper<const Renderable> renderable;
    glm::mat4 model_matrix{};
};

struct ColorDrawCall {
    std::reference_wrapper<const Renderable> renderable;
    glm::mat4 model_matrix;
    glm::vec3 color;
};

}  // namespace Luminol::Graphics
