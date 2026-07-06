#include "SDL_GPUModel.hpp"

#include <LuminolRenderEngine/Utilities/ModelLoader.hpp>
#include <LuminolRenderEngine/Utilities/ImageLoader.hpp>

namespace {

using namespace Luminol::Graphics::SDL_GPU;

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

namespace Luminol::Graphics::SDL_GPU {

SDL_GPUModel::SDL_GPUModel(
    GPUDevice& device, const std::filesystem::path& model_path
) {
    const auto model_data_opt =
        Luminol::Utilities::ModelLoader::load_model(model_path);

    Expects(model_data_opt.has_value());

    const auto& model_data = model_data_opt.value();

    this->meshes.reserve(model_data.meshes.size());

    for (const auto& mesh_data : model_data.meshes) {
        Expects(
            mesh_data.vertices.size() == mesh_data.texture_coordinates.size()
        );

        constexpr auto vertex_components = 11;

        auto mesh_vertices = std::vector<float>{};
        mesh_vertices.reserve(mesh_data.vertices.size() * vertex_components);

        for (size_t i = 0; i < mesh_data.vertices.size(); ++i) {
            mesh_vertices.push_back(mesh_data.vertices[i].x());
            mesh_vertices.push_back(mesh_data.vertices[i].y());
            mesh_vertices.push_back(mesh_data.vertices[i].z());
            mesh_vertices.push_back(mesh_data.texture_coordinates[i].x());
            mesh_vertices.push_back(mesh_data.texture_coordinates[i].y());
            mesh_vertices.push_back(mesh_data.normals[i].x());
            mesh_vertices.push_back(mesh_data.normals[i].y());
            mesh_vertices.push_back(mesh_data.normals[i].z());
            mesh_vertices.push_back(mesh_data.tangents[i].x());
            mesh_vertices.push_back(mesh_data.tangents[i].y());
            mesh_vertices.push_back(mesh_data.tangents[i].z());
        }

        const auto diffuse_texture_image = load_first_texture_or_nothing(
            mesh_data.diffuse_texture_paths, model_data.textures_map
        );

        this->meshes.emplace_back(
            device, mesh_vertices, mesh_data.indices, diffuse_texture_image
        );
    }
}

auto SDL_GPUModel::draw() const -> void {
    for (const auto& mesh : this->meshes) {
        mesh.draw();
    }
}

auto SDL_GPUModel::draw_instanced(int32_t instance_count) const -> void {
    for (const auto& mesh : this->meshes) {
        mesh.draw_instanced(instance_count);
    }
}

auto SDL_GPUModel::draw(RenderPass& sdl_gpu_pass) const -> void {
    for (const auto& mesh : this->meshes) {
        mesh.draw(sdl_gpu_pass);
    }
}

auto SDL_GPUModel::draw_instanced(
    int32_t instance_count, RenderPass& sdl_gpu_pass
) const -> void {
    for (const auto& mesh : this->meshes) {
        mesh.draw_instanced(instance_count, sdl_gpu_pass);
    }
}

}  // namespace Luminol::Graphics::SDL_GPU
