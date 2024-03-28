#include "OpenGLVertexBuffer.hpp"

#include <gsl/gsl>
#include <glad/gl.h>

namespace Luminol::Graphics {

OpenGLVertexBuffer::OpenGLVertexBuffer(gsl::span<const float> vertices) {
    glCreateBuffers(1, &this->vertex_buffer_id);
    glNamedBufferStorage(
        this->vertex_buffer_id,
        gsl::narrow<GLsizeiptr>(vertices.size_bytes()),
        vertices.data(),
        0
    );
}

OpenGLVertexBuffer::~OpenGLVertexBuffer() {
    glDeleteBuffers(1, &this->vertex_buffer_id);
}

auto OpenGLVertexBuffer::get_id() const -> uint32_t {
    return this->vertex_buffer_id;
}

}  // namespace Luminol::Graphics
