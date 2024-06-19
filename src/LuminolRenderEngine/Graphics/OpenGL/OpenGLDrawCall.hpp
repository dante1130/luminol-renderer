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
    std::vector<glm::vec4> colors = {};
};

struct LineDrawCall {
    struct Line {
        glm::vec3 start_position;
        glm::vec3 end_position;
    };

    std::vector<Line> lines = {};
    std::vector<glm::vec4> colors = {};
    std::vector<float> widths = {};
};

}  // namespace Luminol::Graphics
