#pragma once

#include <gsl/gsl>

#include <Engine/Graphics/OpenGL/OpenGLVertexBuffer.hpp>
#include <Engine/Graphics/OpenGL/OpenGLIndexBuffer.hpp>

namespace Luminol::Graphics {

struct VertexAttribute {
    int32_t component_count = {0};
    bool normalized = {false};
    uint32_t relative_offset = {0};
};

class OpenGLVertexArrayObject {
public:
    OpenGLVertexArrayObject(
        gsl::span<const float> buffer,
        gsl::span<const uint32_t> indices,
        gsl::span<const VertexAttribute> attributes
    );
    ~OpenGLVertexArrayObject();
    OpenGLVertexArrayObject(const OpenGLVertexArrayObject&) = delete;
    OpenGLVertexArrayObject(OpenGLVertexArrayObject&&) = default;
    auto operator=(const OpenGLVertexArrayObject&)
        -> OpenGLVertexArrayObject& = delete;
    auto operator=(OpenGLVertexArrayObject&&)
        -> OpenGLVertexArrayObject& = default;

    [[nodiscard]] auto get_index_count() const -> int32_t;

    auto bind() const -> void;
    auto unbind() const -> void;

private:
    uint32_t vertex_array_id = {0};
    OpenGLVertexBuffer vertex_buffer;
    OpenGLIndexBuffer index_buffer;
};

}  // namespace Luminol::Graphics
