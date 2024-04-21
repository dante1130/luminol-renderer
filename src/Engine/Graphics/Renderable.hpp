#pragma once

#include <functional>

namespace Luminol::Graphics {

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
struct Material {
    float shininess = 32.0f;
};
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)

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
    [[nodiscard]] auto get_material() const -> Material;
    auto set_material(const Material& material) -> void;

private:
    Material material;
};

}  // namespace Luminol::Graphics
