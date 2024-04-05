#pragma once

#include <Engine/Graphics/Renderer.hpp>

namespace Luminol::Graphics {

class Model {
public:
    Model() = default;
    virtual ~Model() = default;
    Model(const Model&) = default;
    Model(Model&&) = default;
    auto operator=(const Model&) -> Model& = default;
    auto operator=(Model&&) -> Model& = default;

    [[nodiscard]] virtual auto get_render_command() const -> RenderCommand = 0;
};

}  // namespace Luminol::Graphics
