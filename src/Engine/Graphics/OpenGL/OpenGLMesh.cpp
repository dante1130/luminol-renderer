#pragma once

#include <Engine/Graphics/OpenGL/OpenGLMesh.hpp>
#include <Engine/Graphics/OpenGL/OpenGLTextureManager.hpp>

#include <glad/gl.h>

namespace {

using namespace Luminol::Graphics;

constexpr auto create_vertex_attributes() {
    constexpr auto texture_coordinates_offset = 3 * sizeof(float);
    constexpr auto normal_offset = 5 * sizeof(float);
    constexpr auto tangent_offset = 8 * sizeof(float);

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
        VertexAttribute{
            .component_count = 3,
            .normalized = false,
            .relative_offset = tangent_offset,
        },
    };
}

auto load_texture_from_path(
    const std::optional<std::filesystem::path>& path, ColorSpace color_space
) -> OpenGLMesh::TextureRefOptional {
    if (!path.has_value()) {
        return std::nullopt;
    }

    const auto& path_value = path.value();

    auto& texture_manager = OpenGLTextureManager::get_instance();
    texture_manager.load_texture(path_value, color_space);
    return texture_manager.get_texture(path_value);
}

auto load_texture_from_image(
    const std::optional<Luminol::Utilities::ImageLoader::Image>& image,
    ColorSpace color_space
) -> OpenGLMesh::TextureRefOptional {
    if (!image.has_value()) {
        return std::nullopt;
    }

    const auto& image_value = image.value();

    auto& texture_manager = OpenGLTextureManager::get_instance();
    texture_manager.load_texture(image_value, color_space);
    return texture_manager.get_texture(image_value.path);
}

}  // namespace

namespace Luminol::Graphics {

OpenGLMesh::OpenGLMesh(
    gsl::span<const float> vertices, gsl::span<const uint32_t> indices
)
    : vertex_array_object{vertices, indices, create_vertex_attributes()} {}

OpenGLMesh::OpenGLMesh(
    gsl::span<const float> vertices,
    gsl::span<const uint32_t> indices,
    const TexturePaths& texture_paths
)
    : vertex_array_object{vertices, indices, create_vertex_attributes()},
      diffuse_texture{load_texture_from_path(
          texture_paths.diffuse_texture_path, ColorSpace::SRGB
      )},
      specular_texture{load_texture_from_path(
          texture_paths.specular_texture_path, ColorSpace::Linear
      )},
      emissive_texture{load_texture_from_path(
          texture_paths.emissive_texture_path, ColorSpace::SRGB
      )},
      normal_texture{load_texture_from_path(
          texture_paths.normal_texture_path, ColorSpace::Linear
      )} {}

OpenGLMesh::OpenGLMesh(
    gsl::span<const float> vertices,
    gsl::span<const uint32_t> indices,
    const TextureImages& texture_images
)
    : vertex_array_object{vertices, indices, create_vertex_attributes()},
      diffuse_texture{load_texture_from_image(
          texture_images.diffuse_texture, ColorSpace::SRGB
      )},
      specular_texture{load_texture_from_image(
          texture_images.specular_texture, ColorSpace::Linear
      )},
      emissive_texture{load_texture_from_image(
          texture_images.emissive_texture, ColorSpace::SRGB
      )},
      normal_texture{load_texture_from_image(
          texture_images.normal_texture, ColorSpace::Linear
      )} {}

auto OpenGLMesh::draw() const -> void {
    if (this->diffuse_texture.has_value()) {
        this->diffuse_texture->get().bind(SamplerBindingPoint::TextureDiffuse);
    }

    if (this->specular_texture.has_value()) {
        this->specular_texture->get().bind(SamplerBindingPoint::TextureSpecular
        );
    }

    if (this->emissive_texture.has_value()) {
        this->emissive_texture->get().bind(SamplerBindingPoint::TextureEmissive
        );
    }

    if (this->normal_texture.has_value()) {
        this->normal_texture->get().bind(SamplerBindingPoint::TextureNormal);
    }

    this->vertex_array_object.bind();
    glDrawElements(
        GL_TRIANGLES,
        this->vertex_array_object.get_index_count(),
        GL_UNSIGNED_INT,
        nullptr
    );

    if (this->emissive_texture.has_value()) {
        this->emissive_texture->get().unbind(
            SamplerBindingPoint::TextureEmissive
        );
    }

    if (this->specular_texture.has_value()) {
        this->specular_texture->get().unbind(
            SamplerBindingPoint::TextureSpecular
        );
    }

    if (this->diffuse_texture.has_value()) {
        this->diffuse_texture->get().unbind(SamplerBindingPoint::TextureDiffuse
        );
    }

    if (this->normal_texture.has_value()) {
        this->normal_texture->get().unbind(SamplerBindingPoint::TextureNormal);
    }
}

}  // namespace Luminol::Graphics
