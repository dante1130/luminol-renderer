#pragma once

#include <Engine/Graphics/Mesh.hpp>
#include <Engine/Graphics/OpenGL/OpenGLVertexArrayObject.hpp>

namespace Luminol::Graphics {

class OpenGLMesh : public Mesh {
public:
    OpenGLMesh(gsl::span<const float> vertices);

    [[nodiscard]] auto get_render_command(const Renderer& renderer) const
        -> RenderCommand override;

private:
    OpenGLVertexArrayObject vertex_array_object;
};

}  // namespace Luminol::Graphics
