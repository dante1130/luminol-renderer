#pragma once

#include <cstdint>

namespace Luminol::Graphics {

using RenderableId = uint32_t;

class Renderable {
public:
    Renderable() = default;
    virtual ~Renderable() = default;
    Renderable(const Renderable&) = delete;
    Renderable(Renderable&&) = default;
    auto operator=(const Renderable&) -> Renderable& = delete;
    auto operator=(Renderable&&) -> Renderable& = default;

    virtual auto draw() const -> void = 0;
    virtual auto draw_instanced(int32_t instance_count) const -> void = 0;
};

}  // namespace Luminol::Graphics
