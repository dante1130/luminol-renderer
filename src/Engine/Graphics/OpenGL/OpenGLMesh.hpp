#pragma once

#include <Engine/Graphics/TexturePaths.hpp>
#include <Engine/Graphics/Mesh.hpp>
#include <Engine/Graphics/OpenGL/OpenGLVertexArrayObject.hpp>
#include <Engine/Graphics/OpenGL/OpenGLTexture.hpp>
#include <Engine/Utilities/ImageLoader.hpp>

namespace Luminol::Graphics {

struct TextureImages {
    std::optional<Utilities::ImageLoader::Image> diffuse_texture;
    std::optional<Utilities::ImageLoader::Image> emissive_texture;
    std::optional<Utilities::ImageLoader::Image> normal_texture;
};

class OpenGLMesh : public Mesh {
public:
    using TextureRefOptional =
        std::optional<std::reference_wrapper<const OpenGLTexture>>;

    OpenGLMesh(
        gsl::span<const float> vertices, gsl::span<const uint32_t> indices
    );

    OpenGLMesh(
        gsl::span<const float> vertices,
        gsl::span<const uint32_t> indices,
        const TexturePaths& texture_paths
    );

    OpenGLMesh(
        gsl::span<const float> vertices,
        gsl::span<const uint32_t> indices,
        const TextureImages& texture_images
    );

    auto draw() const -> void override;

private:
    OpenGLVertexArrayObject vertex_array_object;

    TextureRefOptional diffuse_texture = std::nullopt;
    TextureRefOptional emissive_texture = std::nullopt;
    TextureRefOptional normal_texture = std::nullopt;
};

}  // namespace Luminol::Graphics
