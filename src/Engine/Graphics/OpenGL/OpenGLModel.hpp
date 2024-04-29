#pragma once

#include <filesystem>

#include <Engine/Graphics/Model.hpp>
#include <Engine/Graphics/OpenGL/OpenGLMesh.hpp>

namespace Luminol::Graphics {

class OpenGLModel : public Model {
public:
    OpenGLModel(const std::filesystem::path& model_path);

    auto draw() const -> void override;

private:
    std::vector<OpenGLMesh> meshes;
};

}  // namespace Luminol::Graphics
