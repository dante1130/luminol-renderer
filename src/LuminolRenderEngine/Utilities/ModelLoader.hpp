#pragma once

#include <vector>
#include <optional>
#include <filesystem>

#include <glm/glm.hpp>

#include <LuminolRenderEngine/Utilities/ImageLoader.hpp>

namespace Luminol::Utilities::ModelLoader {

struct MeshData {
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> texture_coordinates;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec3> tangents;
    std::vector<uint32_t> indices;

    std::vector<std::filesystem::path> diffuse_texture_paths;
    std::vector<std::filesystem::path> emissive_texture_paths;
    std::vector<std::filesystem::path> normal_texture_paths;

    std::vector<std::filesystem::path> metallic_texture_paths;
    std::vector<std::filesystem::path> roughness_texture_paths;
    std::vector<std::filesystem::path> ambient_occlusion_texture_paths;
};

struct ModelData {
    std::vector<MeshData> meshes;
    std::unordered_map<std::filesystem::path, ImageLoader::Image> textures_map;
};

auto load_model(const std::filesystem::path& path) -> std::optional<ModelData>;

}  // namespace Luminol::Utilities::ModelLoader
