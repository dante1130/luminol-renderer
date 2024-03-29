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
    };
}

}  // namespace

namespace Luminol::Graphics {

OpenGLMesh::OpenGLMesh(gsl::span<const float> vertices)
    : vertex_array_object(vertices, create_vertex_attributes()) {}

auto OpenGLMesh::get_render_command(const Renderer& /*renderer*/) const
    -> RenderCommand {
    return [this](const Renderer& /*renderer*/) {
        vertex_array_object.bind();
        glDrawArrays(GL_TRIANGLES, 0, 3);
    };
}

}  // namespace Luminol::Graphics
