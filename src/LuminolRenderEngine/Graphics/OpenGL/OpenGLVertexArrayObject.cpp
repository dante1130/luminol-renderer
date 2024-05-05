#include "OpenGLVertexArrayObject.hpp"

#include <numeric>

#include <glad/gl.h>

namespace Luminol::Graphics {

OpenGLVertexArrayObject::OpenGLVertexArrayObject(
    gsl::span<const float> buffer,
    gsl::span<const uint32_t> indices,
    gsl::span<const VertexAttribute> attributes
)
    : vertex_buffer(buffer), index_buffer(indices) {
    glCreateVertexArrays(1, &this->vertex_array_id);

    glVertexArrayElementBuffer(
        this->vertex_array_id, this->index_buffer.get_index_buffer_id()
    );

    const auto components = std::accumulate(
        attributes.begin(),
        attributes.end(),
        0,
        [](auto sum, const auto& attribute) {
            return sum + attribute.component_count;
        }
    );

    glVertexArrayVertexBuffer(
        this->vertex_array_id,
        0,
        this->vertex_buffer.get_vertex_buffer_id(),
        0,
        gsl::narrow_cast<GLsizei>(components * sizeof(float))
    );

    for (size_t i = 0; i < attributes.size(); ++i) {
        glEnableVertexArrayAttrib(
            this->vertex_array_id, gsl::narrow_cast<uint32_t>(i)
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

auto OpenGLVertexArrayObject::get_index_count() const -> int32_t {
    return this->index_buffer.get_index_count();
}

auto OpenGLVertexArrayObject::bind() const -> void {
    glBindVertexArray(this->vertex_array_id);
}

// NOLINTBEGIN(readability-convert-member-functions-to-static)
auto OpenGLVertexArrayObject::unbind() const -> void { glBindVertexArray(0); }
// NOLINTEND(readability-convert-member-functions-to-static)

}  // namespace Luminol::Graphics
