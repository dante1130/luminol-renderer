#pragma once

#include <Engine/Graphics/Mesh.hpp>
#include <Engine/Graphics/OpenGL/OpenGLVertexArrayObject.hpp>
#include <Engine/Graphics/OpenGL/OpenGLTexture.hpp>

namespace Luminol::Graphics {

class OpenGLMesh : public Mesh {
public:
    OpenGLMesh(
        gsl::span<const float> vertices,
        gsl::span<const uint32_t> indices,
        const std::filesystem::path& texture_path
    );

    [[nodiscard]] auto get_render_command(const Renderer& renderer) const
        -> RenderCommand override;

private:
    OpenGLVertexArrayObject vertex_array_object;
    OpenGLTexture texture;
};

}  // namespace Luminol::Graphics
