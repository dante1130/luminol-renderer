#pragma once

#include <glm/glm.hpp>

#include <Engine/Window/Window.hpp>
#include <Engine/Graphics/Shader.hpp>
#include <Engine/Graphics/BufferBit.hpp>
#include <Engine/Graphics/OpenGL/OpenGLVertexArrayObject.hpp>

namespace Luminol::Graphics {

struct Drawable {
    std::unique_ptr<OpenGLVertexArrayObject> vertex_array_object = {nullptr};
    std::unique_ptr<Shader> shader = {nullptr};
    int32_t vertex_count = {0};
};

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
    virtual auto draw(const Drawable& drawable) const -> void = 0;
    [[nodiscard]] virtual auto test_draw() const -> Drawable = 0;
};

}  // namespace Luminol::Graphics
