#pragma once

#include <gsl/gsl>

namespace Luminol::Graphics {

class OpenGLVertexBuffer {
public:
    OpenGLVertexBuffer(gsl::span<const float> vertices);
    ~OpenGLVertexBuffer();
    OpenGLVertexBuffer(const OpenGLVertexBuffer&) = delete;
    OpenGLVertexBuffer(OpenGLVertexBuffer&&) = default;
    auto operator=(const OpenGLVertexBuffer&) -> OpenGLVertexBuffer& = delete;
    auto operator=(OpenGLVertexBuffer&&) -> OpenGLVertexBuffer& = default;

    auto bind() const -> void;
    auto unbind() const -> void;

private:
    uint32_t vertex_buffer_id = {0};
};

}  // namespace Luminol::Graphics
