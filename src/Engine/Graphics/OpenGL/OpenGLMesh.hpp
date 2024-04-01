#pragma once

#include <Engine/Graphics/Mesh.hpp>
#include <Engine/Graphics/OpenGL/OpenGLVertexArrayObject.hpp>
#include <Engine/Graphics/OpenGL/OpenGLTexture.hpp>
#include <Engine/Utilities/ImageLoader.hpp>

namespace Luminol::Graphics {

class OpenGLMesh : public Mesh {
public:
    OpenGLMesh(
        gsl::span<const float> vertices,
        gsl::span<const uint32_t> indices,
        const std::filesystem::path& texture_path
    );

    OpenGLMesh(
        gsl::span<const float> vertices,
        gsl::span<const uint32_t> indices,
        const Utilities::ImageLoader::Image& texture_image
    );

    OpenGLMesh(
        gsl::span<const float> vertices, gsl::span<const uint32_t> indices
    );

    [[nodiscard]] auto get_render_command(const Renderer& renderer) const
        -> RenderCommand override;

private:
    OpenGLVertexArrayObject vertex_array_object;
    std::optional<OpenGLTexture> texture = std::nullopt;
};

}  // namespace Luminol::Graphics
