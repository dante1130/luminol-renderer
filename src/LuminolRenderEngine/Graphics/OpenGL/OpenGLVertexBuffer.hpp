#pragma once

#include <gsl/gsl>

namespace Luminol::Graphics {

class OpenGLVertexBuffer {
public:
    OpenGLVertexBuffer(gsl::span<const float> buffer);
    ~OpenGLVertexBuffer();
    OpenGLVertexBuffer(const OpenGLVertexBuffer&) = delete;
    OpenGLVertexBuffer(OpenGLVertexBuffer&&) = default;
    auto operator=(const OpenGLVertexBuffer&) -> OpenGLVertexBuffer& = delete;
    auto operator=(OpenGLVertexBuffer&&) -> OpenGLVertexBuffer& = default;

    auto bind() const -> void;

    [[nodiscard]] auto get_vertex_buffer_id() const -> uint32_t;

private:
    uint32_t vertex_buffer_id = {0};
};

}  // namespace Luminol::Graphics
