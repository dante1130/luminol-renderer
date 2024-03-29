#include "OpenGLVertexArrayObject.hpp"

#include <glad/gl.h>

namespace Luminol::Graphics {

OpenGLVertexArrayObject::OpenGLVertexArrayObject(
    gsl::span<const float> buffer, gsl::span<const VertexAttribute> attributes
)
    : vertex_buffer(buffer) {
    glCreateVertexArrays(1, &this->vertex_array_id);

    for (size_t i = 0; i < attributes.size(); ++i) {
        glEnableVertexArrayAttrib(
            this->vertex_array_id, gsl::narrow_cast<uint32_t>(i)
        );

        glVertexArrayVertexBuffer(
            this->vertex_array_id,
            0,
            this->vertex_buffer.get_id(),
            0,
            gsl::narrow_cast<GLsizei>(
                attributes[i].component_count * sizeof(float)
            )
        );

        glVertexArrayAttribFormat(
            this->vertex_array_id,
            gsl::narrow_cast<uint32_t>(i),
            attributes[i].component_count,
            GL_FLOAT,
            attributes[i].normalized ? GL_TRUE : GL_FALSE,
            attributes[i].relative_offset
        );

        glVertexArrayAttribBinding(
            this->vertex_array_id, gsl::narrow_cast<uint32_t>(i), 0
        );
    }
}

OpenGLVertexArrayObject::~OpenGLVertexArrayObject() {
    glDeleteVertexArrays(1, &this->vertex_array_id);
}

auto OpenGLVertexArrayObject::bind() const -> void {
    glBindVertexArray(this->vertex_array_id);
}

// NOLINTBEGIN(readability-convert-member-functions-to-static)
auto OpenGLVertexArrayObject::unbind() const -> void { glBindVertexArray(0); }
// NOLINTEND(readability-convert-member-functions-to-static)

}  // namespace Luminol::Graphics
