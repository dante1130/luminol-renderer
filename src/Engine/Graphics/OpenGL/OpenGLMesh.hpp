#pragma once

#include <Engine/Graphics/TexturePaths.hpp>
#include <Engine/Graphics/Mesh.hpp>
#include <Engine/Graphics/OpenGL/OpenGLVertexArrayObject.hpp>
#include <Engine/Graphics/OpenGL/OpenGLTexture.hpp>
#include <Engine/Utilities/ImageLoader.hpp>

namespace Luminol::Graphics {

struct TextureImages {
    std::optional<Utilities::ImageLoader::Image> diffuse_texture;
    std::optional<Utilities::ImageLoader::Image> specular_texture;
    std::optional<Utilities::ImageLoader::Image> emissive_texture;
};

class OpenGLMesh : public Mesh {
public:
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

    [[nodiscard]] auto get_render_command(const Renderer& renderer) const
        -> RenderCommand override;

private:
    OpenGLVertexArrayObject vertex_array_object;
    std::optional<OpenGLTexture> diffuse_texture = std::nullopt;
};

}  // namespace Luminol::Graphics
