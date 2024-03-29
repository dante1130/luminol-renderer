#pragma once

#include <Engine/Graphics/Renderer.hpp>

namespace Luminol::Graphics {

class Mesh {
public:
    Mesh() = default;
    virtual ~Mesh() = default;
    Mesh(const Mesh&) = delete;
    Mesh(Mesh&&) = default;
    auto operator=(const Mesh&) -> Mesh& = delete;
    auto operator=(Mesh&&) -> Mesh& = default;

    [[nodiscard]] virtual auto get_render_command(const Renderer& renderer
    ) const -> RenderCommand = 0;
};

}  // namespace Luminol::Graphics
