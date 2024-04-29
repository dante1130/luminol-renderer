#pragma once

#include <filesystem>
#include <optional>

namespace Luminol::Graphics {

struct TexturePaths {
    std::optional<std::filesystem::path> diffuse_texture_path;
    std::optional<std::filesystem::path> emissive_texture_path;
    std::optional<std::filesystem::path> normal_texture_path;
    std::optional<std::filesystem::path> metallic_texture_path;
    std::optional<std::filesystem::path> roughness_texture_path;
    std::optional<std::filesystem::path> ambient_occlusion_texture_path;
};

}  // namespace Luminol::Graphics
