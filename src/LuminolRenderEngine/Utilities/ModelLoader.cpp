#include "ModelLoader.hpp"

#include <array>
#include <string_view>

#include <gsl/gsl>

#include <assimp/GltfMaterial.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace {

using namespace Luminol::Utilities::ImageLoader;
using namespace Luminol::Utilities::ModelLoader;

auto load_vertices(const aiMesh& mesh)
    -> std::vector<Luminol::Maths::Vector3f> {
    auto vertices = std::vector<Luminol::Maths::Vector3f>{};
    vertices.reserve(mesh.mNumVertices);

    const auto vertices_span =
        gsl::make_span(mesh.mVertices, mesh.mNumVertices);

    for (const auto& vertex : vertices_span) {
        vertices.emplace_back(vertex.x, vertex.y, vertex.z);
    }

    return vertices;
}

auto load_texture_coordinates(const aiMesh& mesh)
    -> std::vector<Luminol::Maths::Vector2f> {
    if (!mesh.HasTextureCoords(0)) {
        return std::vector<Luminol::Maths::Vector2f>{mesh.mNumVertices};
    }

    auto texture_coordinates = std::vector<Luminol::Maths::Vector2f>{};
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

auto load_normals(const aiMesh& mesh) -> std::vector<Luminol::Maths::Vector3f> {
    if (!mesh.HasNormals()) {
        return std::vector<Luminol::Maths::Vector3f>{mesh.mNumVertices};
    }

    auto normals = std::vector<Luminol::Maths::Vector3f>{};
    normals.reserve(mesh.mNumVertices);

    const auto normals_span = gsl::make_span(mesh.mNormals, mesh.mNumVertices);

    for (const auto& normal : normals_span) {
        normals.emplace_back(normal.x, normal.y, normal.z);
    }

    return normals;
}

auto load_tangents(const aiMesh& mesh)
    -> std::vector<Luminol::Maths::Vector3f> {
    if (!mesh.HasTangentsAndBitangents()) {
        return std::vector<Luminol::Maths::Vector3f>{mesh.mNumVertices};
    }

    auto tangents = std::vector<Luminol::Maths::Vector3f>{};
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

auto to_texture_wrap_mode(aiTextureMapMode mode) -> Luminol::Utilities::ModelLoader::TextureWrapMode {
    switch (mode) {
        case aiTextureMapMode_Clamp:
        case aiTextureMapMode_Decal:
            return Luminol::Utilities::ModelLoader::TextureWrapMode::ClampToEdge;
        case aiTextureMapMode_Mirror:
            return Luminol::Utilities::ModelLoader::TextureWrapMode::MirroredRepeat;
        case aiTextureMapMode_Wrap:
        default:
            return Luminol::Utilities::ModelLoader::TextureWrapMode::Repeat;
    }
}

struct LoadedTextures {
    std::vector<std::filesystem::path> paths;
    Luminol::Utilities::ModelLoader::TextureWrap wrap;
};

auto load_textures(
    const aiMesh& mesh,
    gsl::span<aiMaterial*> materials,
    aiTextureType texture_type,
    const std::filesystem::path& directory,
    std::unordered_map<std::filesystem::path, Image>& textures_map
) -> LoadedTextures {
    if (materials.empty()) {
        return LoadedTextures{};
    }

    const auto* const material = materials[mesh.mMaterialIndex];

    const auto textures_count = material->GetTextureCount(texture_type);

    auto texture_paths = std::vector<std::filesystem::path>{};
    texture_paths.reserve(textures_count);

    auto wrap = Luminol::Utilities::ModelLoader::TextureWrap{};

    constexpr auto desired_rgba_channels = int32_t{4};

    for (auto i = 0u; i < textures_count; ++i) {
        auto texture_path = aiString{};
        auto mapmode = std::array<aiTextureMapMode, 3>{
            aiTextureMapMode_Wrap, aiTextureMapMode_Wrap, aiTextureMapMode_Wrap
        };
        material->GetTexture(
            texture_type, i, &texture_path, nullptr, nullptr, nullptr,
            nullptr, mapmode.data()
        );

        const auto texture_path_key = directory / texture_path.C_Str();

        texture_paths.emplace_back(texture_path_key);

        if (i == 0) {
            wrap = Luminol::Utilities::ModelLoader::TextureWrap{
                .u = to_texture_wrap_mode(mapmode[0]),
                .v = to_texture_wrap_mode(mapmode[1]),
            };
        }

        if (!textures_map.contains(texture_path_key)) {
            textures_map[texture_path_key] =
                load_image(texture_path_key, desired_rgba_channels);
        }
    }

    return LoadedTextures{.paths = texture_paths, .wrap = wrap};
}

auto get_material_alpha_mode(
    gsl::span<aiMaterial*> materials, unsigned int material_index
) -> Luminol::Utilities::ModelLoader::AlphaMode {
    using Luminol::Utilities::ModelLoader::AlphaMode;

    if (materials.empty()) {
        return AlphaMode::Opaque;
    }

    const auto* const material = materials[material_index];

    auto alpha_mode_string = aiString{};
    if (material->Get(AI_MATKEY_GLTF_ALPHAMODE, alpha_mode_string) ==
        AI_SUCCESS) {
        const auto mode = std::string_view{alpha_mode_string.C_Str()};
        if (mode == "MASK") {
            return AlphaMode::Mask;
        }
        if (mode == "BLEND") {
            return AlphaMode::Blend;
        }
        return AlphaMode::Opaque;
    }

    // Non-glTF formats (e.g. OBJ) don't expose an alpha mode; approximate
    // blend transparency from material opacity instead.
    constexpr auto opaque_opacity_threshold = 0.999F;
    auto opacity = 1.0F;
    if (material->Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS &&
        opacity < opaque_opacity_threshold) {
        return AlphaMode::Blend;
    }

    return AlphaMode::Opaque;
}

auto load_mesh(
    const aiMesh& mesh,
    gsl::span<aiMaterial*> materials,
    const std::filesystem::path& directory,
    std::unordered_map<std::filesystem::path, Image>& textures_map
) -> MeshData {
    auto diffuse = load_textures(
        mesh, materials, aiTextureType_DIFFUSE, directory, textures_map
    );
    auto emissive = load_textures(
        mesh, materials, aiTextureType_EMISSIVE, directory, textures_map
    );
    auto normal = load_textures(
        mesh, materials, aiTextureType_NORMALS, directory, textures_map
    );
    auto metallic = load_textures(
        mesh, materials, aiTextureType_METALNESS, directory, textures_map
    );
    auto roughness = load_textures(
        mesh, materials, aiTextureType_DIFFUSE_ROUGHNESS, directory,
        textures_map
    );
    auto ambient_occlusion = load_textures(
        mesh, materials, aiTextureType_AMBIENT_OCCLUSION, directory,
        textures_map
    );

    return MeshData{
        .vertices = load_vertices(mesh),
        .texture_coordinates = load_texture_coordinates(mesh),
        .normals = load_normals(mesh),
        .tangents = load_tangents(mesh),
        .indices = load_indices(mesh),
        .diffuse_texture_paths = std::move(diffuse.paths),
        .diffuse_texture_wrap = diffuse.wrap,
        .emissive_texture_paths = std::move(emissive.paths),
        .emissive_texture_wrap = emissive.wrap,
        .normal_texture_paths = std::move(normal.paths),
        .normal_texture_wrap = normal.wrap,
        .metallic_texture_paths = std::move(metallic.paths),
        .metallic_texture_wrap = metallic.wrap,
        .roughness_texture_paths = std::move(roughness.paths),
        .roughness_texture_wrap = roughness.wrap,
        .ambient_occlusion_texture_paths = std::move(ambient_occlusion.paths),
        .ambient_occlusion_texture_wrap = ambient_occlusion.wrap,
        .alpha_mode = get_material_alpha_mode(materials, mesh.mMaterialIndex),
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
