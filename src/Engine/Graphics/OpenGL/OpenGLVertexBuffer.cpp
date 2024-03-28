#include "OpenGLVertexBuffer.hpp"

#include <gsl/gsl>
#include <glad/gl.h>

namespace Luminol::Graphics {

OpenGLVertexBuffer::OpenGLVertexBuffer(gsl::span<const float> vertices) {
    glGenBuffers(1, &this->vertex_buffer_id);
    this->bind();
    glNamedBufferData(
        this->vertex_buffer_id,
        gsl::narrow<GLsizeiptr>(vertices.size_bytes()),
        vertices.data(),
        GL_STATIC_DRAW
    );
}

OpenGLVertexBuffer::~OpenGLVertexBuffer() {
    glDeleteBuffers(1, &this->vertex_buffer_id);
}

auto OpenGLVertexBuffer::bind() const -> void {
    glBindBuffer(GL_ARRAY_BUFFER, this->vertex_buffer_id);
}

// NOLINTBEGIN(readability-convert-member-functions-to-static)
auto OpenGLVertexBuffer::unbind() const -> void {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
// NOLINTEND(readability-convert-member-functions-to-static)

}  // namespace Luminol::Graphics
