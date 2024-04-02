#include "OpenGLModel.hpp"

#include <Engine/Utilities/ModelLoader.hpp>

namespace Luminol::Graphics {

OpenGLModel::OpenGLModel(const std::filesystem::path& model_path) {
    const auto model_data_opt =
        Luminol::Utilities::ModelLoader::load_model(model_path);

    Expects(model_data_opt.has_value());

    const auto& model_data = model_data_opt.value();

    this->meshes.reserve(model_data.meshes.size());

    for (const auto& mesh_data : model_data.meshes) {
        Expects(
            mesh_data.vertices.size() == mesh_data.texture_coordinates.size()
        );

        constexpr auto vertex_components = 8;

        auto mesh_vertices = std::vector<float>{};
        mesh_vertices.reserve(mesh_data.vertices.size() * vertex_components);

        for (size_t i = 0; i < mesh_data.vertices.size(); ++i) {
            mesh_vertices.push_back(mesh_data.vertices[i].x);
            mesh_vertices.push_back(mesh_data.vertices[i].y);
            mesh_vertices.push_back(mesh_data.vertices[i].z);
            mesh_vertices.push_back(mesh_data.texture_coordinates[i].x);
            mesh_vertices.push_back(mesh_data.texture_coordinates[i].y);
            mesh_vertices.push_back(mesh_data.normals[i].x);
            mesh_vertices.push_back(mesh_data.normals[i].y);
            mesh_vertices.push_back(mesh_data.normals[i].z);
        }

        if (mesh_data.diffuse_texture_paths.empty()) {
            this->meshes.emplace_back(std::make_unique<OpenGLMesh>(
                gsl::make_span(mesh_vertices), gsl::make_span(mesh_data.indices)
            ));
        } else {
            const auto& diffuse_texture =
                model_data.textures_map.at(mesh_data.diffuse_texture_paths[0]);

            this->meshes.emplace_back(std::make_unique<OpenGLMesh>(
                gsl::make_span(mesh_vertices),
                gsl::make_span(mesh_data.indices),
                diffuse_texture
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
