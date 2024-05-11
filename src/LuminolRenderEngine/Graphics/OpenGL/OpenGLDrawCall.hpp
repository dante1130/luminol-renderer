#pragma once

#include <vector>

#include <glm/glm.hpp>

#include <LuminolRenderEngine/Graphics/Renderable.hpp>

namespace Luminol::Graphics {

struct InstancedDrawCall {
    std::reference_wrapper<const Renderable> renderable;
    glm::mat4 model_matrix{};
};

struct ColorInstancedDrawCall {
    RenderableId renderable_id;
    std::vector<glm::mat4> model_matrices;
    std::vector<glm::vec3> colors;
};

}  // namespace Luminol::Graphics
