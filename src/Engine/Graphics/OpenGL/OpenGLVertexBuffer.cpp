#include "OpenGLVertexBuffer.hpp"

#include <gsl/gsl>
#include <glad/gl.h>

namespace Luminol::Graphics {

OpenGLVertexBuffer::OpenGLVertexBuffer(gsl::span<const float> vertices)
    : VertexBuffer(vertices) {
    glGenBuffers(1, &vertex_buffer_id);
    this->bind();
    glNamedBufferData(
        vertex_buffer_id,
        gsl::narrow<GLsizeiptr>(vertices.size_bytes()),
        vertices.data(),
        GL_STATIC_DRAW
    );
}

OpenGLVertexBuffer::~OpenGLVertexBuffer() {
    glDeleteBuffers(1, &vertex_buffer_id);
}

auto OpenGLVertexBuffer::bind() const -> void {
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_id);
}

auto OpenGLVertexBuffer::unbind() const -> void {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

}  // namespace Luminol::Graphics
