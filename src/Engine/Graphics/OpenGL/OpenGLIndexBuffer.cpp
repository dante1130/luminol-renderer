#include "OpenGLIndexBuffer.hpp"

#include <glad/gl.h>

namespace Luminol::Graphics {

OpenGLIndexBuffer::OpenGLIndexBuffer(gsl::span<const uint32_t> indices)
    : index_count{gsl::narrow<int32_t>(indices.size())} {
    glCreateBuffers(1, &this->index_buffer_id);
    glNamedBufferStorage(
        this->index_buffer_id,
        gsl::narrow<GLsizeiptr>(indices.size_bytes()),
        indices.data(),
        0
    );
}

OpenGLIndexBuffer::~OpenGLIndexBuffer() {
    glDeleteBuffers(1, &this->index_buffer_id);
}

auto OpenGLIndexBuffer::get_index_buffer_id() const -> uint32_t {
    return this->index_buffer_id;
}

auto OpenGLIndexBuffer::get_index_count() const -> int32_t {
    return this->index_count;
}

}  // namespace Luminol::Graphics
