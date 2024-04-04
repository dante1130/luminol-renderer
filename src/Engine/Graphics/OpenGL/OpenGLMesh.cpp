#pragma once

#include <Engine/Graphics/OpenGL/OpenGLMesh.hpp>

#include <glad/gl.h>

namespace {

using namespace Luminol::Graphics;

constexpr auto create_vertex_attributes() {
    constexpr auto texture_coordinates_offset = 3 * sizeof(float);
    constexpr auto normal_offset = 5 * sizeof(float);

    return std::array{
        VertexAttribute{
            .component_count = 3,
            .normalized = false,
            .relative_offset = 0,
        },
        VertexAttribute{
            .component_count = 2,
            .normalized = false,
            .relative_offset = texture_coordinates_offset,
        },
        VertexAttribute{
            .component_count = 3,
            .normalized = false,
            .relative_offset = normal_offset,
        },
    };
}

}  // namespace

namespace Luminol::Graphics {

OpenGLMesh::OpenGLMesh(
    gsl::span<const float> vertices, gsl::span<const uint32_t> indices
)
    : vertex_array_object(vertices, indices, create_vertex_attributes()) {}

OpenGLMesh::OpenGLMesh(
    gsl::span<const float> vertices,
    gsl::span<const uint32_t> indices,
    const TexturePaths& texture_paths
)
    : vertex_array_object(vertices, indices, create_vertex_attributes()),
      diffuse_texture(texture_paths.diffuse_texture_path) {}

OpenGLMesh::OpenGLMesh(
    gsl::span<const float> vertices,
    gsl::span<const uint32_t> indices,
    const TextureImages& texture_images
)
    : vertex_array_object(vertices, indices, create_vertex_attributes()),
      diffuse_texture(texture_images.diffuse_texture) {}

auto OpenGLMesh::get_render_command(const Renderer& /*renderer*/) const
    -> RenderCommand {
    return [this](const Renderer& /*renderer*/) {
        if (this->diffuse_texture.has_value()) {
            this->diffuse_texture->bind(SamplerBindingPoint::TextureDiffuse);
        }

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
