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

auto load_normals(const aiMesh& mesh) -> std::vector<glm::vec3> {
    if (!mesh.HasNormals()) {
        return std::vector<glm::vec3>{mesh.mNumVertices, glm::vec3{0.0f}};
    }

    auto normals = std::vector<glm::vec3>{};
    normals.reserve(mesh.mNumVertices);

    const auto normals_span = gsl::make_span(mesh.mNormals, mesh.mNumVertices);

    for (const auto& normal : normals_span) {
        normals.emplace_back(normal.x, normal.y, normal.z);
    }

    return normals;
}

auto load_tangents(const aiMesh& mesh) -> std::vector<glm::vec3> {
    if (!mesh.HasTangentsAndBitangents()) {
        return std::vector<glm::vec3>{mesh.mNumVertices, glm::vec3{0.0f}};
    }

    auto tangents = std::vector<glm::vec3>{};
    tangents.reserve(mesh.mNumVertices);

    const auto tangents_span =
        gsl::make_span(mesh.mTangents, mesh.mNumVertices);

    for (const auto& tangent : tangents_span) {
        tangents.emplace_back(tangent.x, tangent.y, tangent.z);
    }

    return tangents;
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

auto load_textures(
    const aiMesh& mesh,
    gsl::span<aiMaterial*> materials,
    aiTextureType texture_type,
    const std::filesystem::path& directory,
    std::unordered_map<std::filesystem::path, Image>& textures_map
) -> std::vector<std::filesystem::path> {
    if (materials.empty()) {
        return std::vector<std::filesystem::path>{};
    }

    const auto* const material = materials[mesh.mMaterialIndex];

    const auto textures_count = material->GetTextureCount(texture_type);

    auto texture_paths = std::vector<std::filesystem::path>{};
    texture_paths.reserve(textures_count);

    for (auto i = 0u; i < textures_count; ++i) {
        auto texture_path = aiString{};
        material->GetTexture(texture_type, i, &texture_path);

        const auto texture_path_key = directory / texture_path.C_Str();

        texture_paths.emplace_back(texture_path_key);

        if (!textures_map.contains(texture_path_key)) {
            textures_map[texture_path_key] = load_image(texture_path_key);
        }
    }

    return texture_paths;
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
        .normals = load_normals(mesh),
        .tangents = load_tangents(mesh),
        .indices = load_indices(mesh),
        .diffuse_texture_paths = load_textures(
            mesh, materials, aiTextureType_DIFFUSE, directory, textures_map
        ),
        .emissive_texture_paths = load_textures(
            mesh, materials, aiTextureType_EMISSIVE, directory, textures_map
        ),
        .normal_texture_paths = load_textures(
            mesh, materials, aiTextureType_NORMALS, directory, textures_map
        ),
        .metallic_texture_paths = load_textures(
            mesh, materials, aiTextureType_METALNESS, directory, textures_map
        ),
        .roughness_texture_paths = load_textures(
            mesh,
            materials,
            aiTextureType_DIFFUSE_ROUGHNESS,
            directory,
            textures_map
        ),
        .ambient_occlusion_texture_paths = load_textures(
            mesh,
            materials,
            aiTextureType_AMBIENT_OCCLUSION,
            directory,
            textures_map
        )
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
            aiProcess_FlipUVs | aiProcess_PreTransformVertices |
            aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace |
            aiProcess_OptimizeMeshes | aiProcess_SortByPType |
            aiProcess_RemoveRedundantMaterials | aiProcess_ImproveCacheLocality
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
