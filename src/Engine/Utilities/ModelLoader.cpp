#include "ModelLoader.hpp"

#include <gsl/gsl>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace {

using namespace Luminol::Utilities::ImageLoader;
using namespace Luminol::Utilities::ModelLoader;

auto load_vertices(const aiMesh& mesh) -> std::vector<glm::vec3> {
    auto vertices = std::vector<glm::vec3>{};
    vertices.reserve(mesh.mNumVertices);

    const auto vertices_span =
        gsl::make_span(mesh.mVertices, mesh.mNumVertices);

    for (const auto& vertex : vertices_span) {
        vertices.emplace_back(vertex.x, vertex.y, vertex.z);
    }

    return vertices;
}

auto load_texture_coordinates(const aiMesh& mesh) -> std::vector<glm::vec2> {
    if (!mesh.HasTextureCoords(0)) {
        return std::vector<glm::vec2>{mesh.mNumVertices, glm::vec2{0.0f}};
    }

    auto texture_coordinates = std::vector<glm::vec2>{};
    texture_coordinates.reserve(mesh.mNumVertices);

    const auto texture_coordinates_span =
        gsl::make_span(mesh.mTextureCoords[0], mesh.mNumVertices);

    for (const auto& texture_coordinate : texture_coordinates_span) {
        texture_coordinates.emplace_back(
            texture_coordinate.x, texture_coordinate.y
        );
    }

    return texture_coordinates;
}

auto load_indices(const aiMesh& mesh) -> std::vector<uint32_t> {
    auto indices = std::vector<uint32_t>{};
    indices.reserve(gsl::narrow<size_t>(mesh.mNumFaces * 3));

    const auto faces_span = gsl::make_span(mesh.mFaces, mesh.mNumFaces);

    for (const auto& face : faces_span) {
        const auto indices_span =
            gsl::make_span(face.mIndices, face.mNumIndices);

        for (const auto index : indices_span) {
            indices.emplace_back(index);
        }
    }

    return indices;
}

auto load_diffuse_textures(
    const aiMesh& mesh,
    gsl::span<aiMaterial*> materials,
    const std::filesystem::path& directory,
    std::unordered_map<std::filesystem::path, Image>& textures_map
) -> std::vector<std::filesystem::path> {
    if (materials.empty()) {
        return std::vector<std::filesystem::path>{};
    }

    const auto* const material = materials[mesh.mMaterialIndex];

    const auto diffuse_textures_count =
        material->GetTextureCount(aiTextureType_DIFFUSE);

    auto diffuse_texture_paths = std::vector<std::filesystem::path>{};
    diffuse_texture_paths.reserve(diffuse_textures_count);

    for (auto i = 0u; i < diffuse_textures_count; ++i) {
        auto texture_path = aiString{};
        material->GetTexture(aiTextureType_DIFFUSE, i, &texture_path);

        const auto texture_path_key = directory / texture_path.C_Str();

        diffuse_texture_paths.emplace_back(texture_path_key);

        if (!textures_map.contains(texture_path_key)) {
            textures_map[texture_path_key] = load_image(texture_path_key);
        }
    }

    return diffuse_texture_paths;
}

auto load_mesh(
    const aiMesh& mesh,
    gsl::span<aiMaterial*> materials,
    const std::filesystem::path& directory,
    std::unordered_map<std::filesystem::path, Image>& textures_map
) -> MeshData {
    return MeshData{
        .vertices = load_vertices(mesh),
        .texture_coordinates = load_texture_coordinates(mesh),
        .indices = load_indices(mesh),
        .diffuse_texture_paths =
            load_diffuse_textures(mesh, materials, directory, textures_map)
    };
}

}  // namespace

namespace Luminol::Utilities::ModelLoader {

auto load_model(const std::filesystem::path& path) -> std::optional<ModelData> {
    auto importer = Assimp::Importer{};
    const auto* const scene = importer.ReadFile(
        path.string(),
        aiProcess_Triangulate | aiProcess_JoinIdenticalVertices |
            aiProcess_MakeLeftHanded | aiProcess_FlipWindingOrder |
            aiProcess_FlipUVs
    );

    if (scene == nullptr || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) == 1) {
        return std::nullopt;
    }

    auto model_data = ModelData{};
    model_data.meshes.reserve(scene->mNumMeshes);

    const auto mesh_span = gsl::make_span(scene->mMeshes, scene->mNumMeshes);

    for (const auto* mesh : mesh_span) {
        const auto materials_span =
            gsl::make_span(scene->mMaterials, scene->mNumMaterials);

        model_data.meshes.emplace_back(load_mesh(
            *mesh, materials_span, path.parent_path(), model_data.textures_map
        ));
    }

    return model_data;
}

}  // namespace Luminol::Utilities::ModelLoader
