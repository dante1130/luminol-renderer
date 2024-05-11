#pragma once

#include <gsl/gsl>
#include <glm/glm.hpp>

#include <LuminolRenderEngine/Graphics/Renderable.hpp>

namespace Luminol::Graphics {

struct InstancedDrawCall {
    std::reference_wrapper<const Renderable> renderable;
    glm::mat4 model_matrix{};
};

struct ColorInstancedDrawCall {
    std::reference_wrapper<const Renderable> renderable;
    gsl::span<glm::mat4> model_matrices;
    gsl::span<glm::vec3> colors;
};

}  // namespace Luminol::Graphics
