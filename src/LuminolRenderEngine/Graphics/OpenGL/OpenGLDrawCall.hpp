#pragma once

#include <vector>

#include <LuminolMaths/Vector.hpp>
#include <LuminolMaths/Matrix.hpp>

#include <LuminolRenderEngine/Graphics/Renderable.hpp>

namespace Luminol::Graphics {

struct InstancedDrawCall {
    RenderableId renderable_id = 0;
    std::vector<Maths::Matrix4x4f> model_matrices = {};
};

struct ColorInstancedDrawCall {
    RenderableId renderable_id = 0;
    std::vector<Maths::Matrix4x4f> model_matrices = {};
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
