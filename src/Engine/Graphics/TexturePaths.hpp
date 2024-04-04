#pragma once

#include <filesystem>
#include <optional>

namespace Luminol::Graphics {

struct TexturePaths {
    std::optional<std::filesystem::path> diffuse_texture_path;
    std::optional<std::filesystem::path> specular_texture_path;
    std::optional<std::filesystem::path> emissive_texture_path;
    std::optional<std::filesystem::path> normal_texture_path;
};

}  // namespace Luminol::Graphics
