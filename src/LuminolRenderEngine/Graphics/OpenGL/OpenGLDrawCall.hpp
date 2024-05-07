#pragma once

#include <gsl/gsl>
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

struct ColorDrawInstancedCall {
    std::reference_wrapper<const Renderable> renderable;
    gsl::span<glm::mat4> model_matrices;
    gsl::span<glm::vec3> colors;
};

}  // namespace Luminol::Graphics
