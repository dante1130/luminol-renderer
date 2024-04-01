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

    std::vector<ImageLoader::Image> diffuse_textures;
};

auto load_model(const std::filesystem::path& path)
    -> std::optional<std::vector<MeshData>>;

}  // namespace Luminol::Utilities::ModelLoader
