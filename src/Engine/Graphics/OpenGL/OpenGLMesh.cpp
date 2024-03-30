#pragma once

#include <Engine/Graphics/OpenGL/OpenGLMesh.hpp>

#include <glad/gl.h>

namespace {

using namespace Luminol::Graphics;

constexpr auto create_vertex_attributes() {
    return std::array{
        VertexAttribute{
            .component_count = 3,
            .normalized = false,
            .relative_offset = 0,
        },
        VertexAttribute{
            .component_count = 2,
            .normalized = false,
            .relative_offset = 3 * sizeof(float),
        },
    };
}

}  // namespace

namespace Luminol::Graphics {

OpenGLMesh::OpenGLMesh(
    gsl::span<const float> vertices,
    gsl::span<const uint32_t> indices,
    const std::filesystem::path& texture_path
)
    : vertex_array_object(vertices, indices, create_vertex_attributes()),
      texture(texture_path) {}

auto OpenGLMesh::get_render_command(const Renderer& /*renderer*/) const
    -> RenderCommand {
    return [this](const Renderer& /*renderer*/) {
        this->texture.bind();
        this->vertex_array_object.bind();
        glDrawElements(
            GL_TRIANGLES,
            this->vertex_array_object.get_index_count(),
            GL_UNSIGNED_INT,
            nullptr
        );
    };
}

}  // namespace Luminol::Graphics