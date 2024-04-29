#pragma once

#include <functional>

namespace Luminol::Graphics {

using RenderCommand = std::function<void()>;

class Renderable {
public:
    Renderable() = default;
    virtual ~Renderable() = default;
    Renderable(const Renderable&) = delete;
    Renderable(Renderable&&) = default;
    auto operator=(const Renderable&) -> Renderable& = delete;
    auto operator=(Renderable&&) -> Renderable& = default;

    [[nodiscard]] virtual auto get_render_command() const -> RenderCommand = 0;
};

}  // namespace Luminol::Graphics
