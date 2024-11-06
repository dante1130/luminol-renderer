#pragma once

#include <vector>

#include <glm/glm.hpp>
#include <LuminolMaths/Vector.hpp>

#include <LuminolRenderEngine/Graphics/Renderable.hpp>

namespace Luminol::Graphics {

struct InstancedDrawCall {
    RenderableId renderable_id = 0;
    std::vector<glm::mat4> model_matrices = {};
};

struct ColorInstancedDrawCall {
    RenderableId renderable_id = 0;
    std::vector<glm::mat4> model_matrices = {};
    std::vector<Maths::Vector4f> colors = {};
};

struct LineDrawCall {
    struct Line {
        Maths::Vector3f start_position;
        Maths::Vector3f end_position;
    };

    std::vector<Line> lines = {};
    std::vector<Maths::Vector4f> colors = {};
};

}  // namespace Luminol::Graphics
