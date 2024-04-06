#pragma once

#include <filesystem>

#include <Engine/Graphics/Model.hpp>
#include <Engine/Graphics/OpenGL/OpenGLMesh.hpp>

namespace Luminol::Graphics {

class OpenGLModel : public Model {
public:
    OpenGLModel(const std::filesystem::path& model_path);

    [[nodiscard]] auto get_render_command() const -> RenderCommand override;

private:
    std::vector<OpenGLMesh> meshes;
};

}  // namespace Luminol::Graphics
