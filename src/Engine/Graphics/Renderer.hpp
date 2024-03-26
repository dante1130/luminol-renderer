#pragma once

#include <glm/glm.hpp>

#include <Engine/Window/Window.hpp>
#include <Engine/Graphics/BufferBit.hpp>

namespace Luminol::Graphics {

class Renderer {
public:
    Renderer() = default;
    virtual ~Renderer() = default;
    Renderer(const Renderer&) = default;
    Renderer(Renderer&&) = delete;
    auto operator=(const Renderer&) -> Renderer& = default;
    auto operator=(Renderer&&) -> Renderer& = delete;

    virtual auto clear_color(const glm::vec4& color) const -> void = 0;
    virtual auto clear(BufferBit buffer_bit) const -> void = 0;
};

}  // namespace Luminol::Graphics
