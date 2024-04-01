#include "OpenGLModel.hpp"

#include <Engine/Utilities/ModelLoader.hpp>

namespace Luminol::Graphics {

OpenGLModel::OpenGLModel(const std::filesystem::path& model_path) {
    const auto meshes_data_opt =
        Luminol::Utilities::ModelLoader::load_model(model_path);

    Expects(meshes_data_opt.has_value());

    const auto& meshes_data = meshes_data_opt.value();

    this->meshes.reserve(meshes_data.size());

    for (const auto& mesh_data : meshes_data) {
        Expects(
            mesh_data.vertices.size() == mesh_data.texture_coordinates.size()
        );

        constexpr auto vertex_components = 5;

        auto mesh_vertices = std::vector<float>{};
        mesh_vertices.reserve(mesh_data.vertices.size() * vertex_components);

        for (size_t i = 0; i < mesh_data.vertices.size(); ++i) {
            mesh_vertices.push_back(mesh_data.vertices[i].x);
            mesh_vertices.push_back(mesh_data.vertices[i].y);
            mesh_vertices.push_back(mesh_data.vertices[i].z);
            mesh_vertices.push_back(mesh_data.texture_coordinates[i].x);
            mesh_vertices.push_back(mesh_data.texture_coordinates[i].y);
        }

        if (mesh_data.diffuse_textures.empty()) {
            this->meshes.emplace_back(std::make_unique<OpenGLMesh>(
                gsl::make_span(mesh_vertices), gsl::make_span(mesh_data.indices)
            ));
        } else {
            this->meshes.emplace_back(std::make_unique<OpenGLMesh>(
                gsl::make_span(mesh_vertices),
                gsl::make_span(mesh_data.indices),
                mesh_data.diffuse_textures[0]
            ));
        }
    }
}

auto OpenGLModel::get_render_command(const Renderer& /*renderer*/) const
    -> RenderCommand {
    return [this](const Renderer& renderer) {
        for (const auto& mesh : this->meshes) {
            mesh->get_render_command(renderer)(renderer);
        }
    };
}

}  // namespace Luminol::Graphics
