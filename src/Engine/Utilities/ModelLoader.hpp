#pragma once

#include <vector>
#include <optional>
#include <filesystem>

#include <glm/glm.hpp>

#include <Engine/Utilities/ImageLoader.hpp>

namespace Luminol::Utilities::ModelLoader {

struct MeshData {
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> texture_coordinates;
    std::vector<uint32_t> indices;

    std::vector<std::filesystem::path> diffuse_texture_paths;
};

struct ModelData {
    std::vector<MeshData> meshes;
    std::unordered_map<std::filesystem::path, ImageLoader::Image> textures_map;
};

auto load_model(const std::filesystem::path& path) -> std::optional<ModelData>;

}  // namespace Luminol::Utilities::ModelLoader
