#pragma once

#include <vector>

#include <glm/glm.hpp>

#include <LuminolRenderEngine/Graphics/Renderable.hpp>

namespace Luminol::Graphics {

struct InstancedDrawCall {
    RenderableId renderable_id = 0;
    std::vector<glm::mat4> model_matrices = {};
};

struct ColorInstancedDrawCall {
    RenderableId renderable_id = 0;
    std::vector<glm::mat4> model_matrices = {};
    std::vector<glm::vec3> colors = {};
};

}  // namespace Luminol::Graphics
