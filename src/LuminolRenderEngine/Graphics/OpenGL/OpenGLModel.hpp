#pragma once

#include <filesystem>

#include <LuminolRenderEngine/Graphics/Model.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLMesh.hpp>

namespace Luminol::Graphics {

class OpenGLModel : public Model {
public:
    OpenGLModel(const std::filesystem::path& model_path);

    auto draw() const -> void override;
    auto draw_instanced(int32_t instance_count) const -> void override;

private:
    std::vector<OpenGLMesh> meshes;
};

}  // namespace Luminol::Graphics
