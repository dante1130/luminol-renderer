#include "OpenGLModel.hpp"

#include <Engine/Utilities/ModelLoader.hpp>
#include "Engine/Utilities/ImageLoader.hpp"

namespace {

using namespace Luminol::Graphics;

auto load_first_texture_or_nothing(
    const std::vector<std::filesystem::path>& texture_paths,
    const std::unordered_map<
        std::filesystem::path,
        Luminol::Utilities::ImageLoader::Image>& textures_map
) -> std::optional<Luminol::Utilities::ImageLoader::Image> {
    if (texture_paths.empty()) {
        return std::nullopt;
    }

    return textures_map.at(texture_paths[0]);
}

}  // namespace

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

        const auto texture_images = TextureImages{
            .diffuse_texture = load_first_texture_or_nothing(
                mesh_data.diffuse_texture_paths, model_data.textures_map
            ),
            .specular_texture = load_first_texture_or_nothing(
                mesh_data.specular_texture_paths, model_data.textures_map
            ),
            .emissive_texture = load_first_texture_or_nothing(
                mesh_data.emissive_texture_paths, model_data.textures_map
            ),
        };

        this->meshes.emplace_back(std::make_unique<OpenGLMesh>(
            mesh_vertices, mesh_data.indices, texture_images
        ));
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
