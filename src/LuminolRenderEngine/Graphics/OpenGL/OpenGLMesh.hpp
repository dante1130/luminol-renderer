#pragma once

#include <LuminolRenderEngine/Graphics/TexturePaths.hpp>
#include <LuminolRenderEngine/Graphics/Mesh.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLVertexArrayObject.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLTexture.hpp>
#include <LuminolRenderEngine/Utilities/ImageLoader.hpp>

namespace Luminol::Graphics {

struct TextureImages {
    std::optional<Utilities::ImageLoader::Image> diffuse_texture;
    std::optional<Utilities::ImageLoader::Image> emissive_texture;
    std::optional<Utilities::ImageLoader::Image> normal_texture;
    std::optional<Utilities::ImageLoader::Image> metallic_texture;
    std::optional<Utilities::ImageLoader::Image> roughness_texture;
    std::optional<Utilities::ImageLoader::Image> ambient_occlusion_texture;
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
    auto draw_instanced(int32_t instance_count) const -> void override;

private:
    auto bind_textures() const -> void;
    auto unbind_textures() const -> void;

    OpenGLVertexArrayObject vertex_array_object;

    TextureRefOptional diffuse_texture = std::nullopt;
    TextureRefOptional emissive_texture = std::nullopt;
    TextureRefOptional normal_texture = std::nullopt;
    TextureRefOptional metallic_texture = std::nullopt;
    TextureRefOptional roughness_texture = std::nullopt;
    TextureRefOptional ambient_occlusion_texture = std::nullopt;
};

}  // namespace Luminol::Graphics
