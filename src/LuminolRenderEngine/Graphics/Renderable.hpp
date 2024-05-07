#pragma once

namespace Luminol::Graphics {

class Renderable {
public:
    Renderable() = default;
    virtual ~Renderable() = default;
    Renderable(const Renderable&) = delete;
    Renderable(Renderable&&) = default;
    auto operator=(const Renderable&) -> Renderable& = delete;
    auto operator=(Renderable&&) -> Renderable& = default;

    virtual auto draw() const -> void = 0;
};

}  // namespace Luminol::Graphics
