#pragma once

#include <vector>
#include <optional>
#include <filesystem>
#include <unordered_map>

#include <LuminolMaths/Vector.hpp>

#include <LuminolRenderEngine/Utilities/ImageLoader.hpp>

namespace Luminol::Utilities::ModelLoader {

enum class TextureWrapMode {
    Repeat,
    ClampToEdge,
    MirroredRepeat,
};

enum class AlphaMode {
    Opaque,
    Mask,
    Blend,
};

struct TextureWrap {
    TextureWrapMode u = TextureWrapMode::Repeat;
    TextureWrapMode v = TextureWrapMode::Repeat;
};

struct MeshData {
    std::vector<Luminol::Maths::Vector3f> vertices;
    std::vector<Luminol::Maths::Vector2f> texture_coordinates;
    std::vector<Luminol::Maths::Vector3f> normals;
    std::vector<Luminol::Maths::Vector3f> tangents;
    std::vector<uint32_t> indices;

    std::vector<std::filesystem::path> diffuse_texture_paths;
    TextureWrap diffuse_texture_wrap;
    std::vector<std::filesystem::path> emissive_texture_paths;
    TextureWrap emissive_texture_wrap;
    std::vector<std::filesystem::path> normal_texture_paths;
    TextureWrap normal_texture_wrap;

    std::vector<std::filesystem::path> metallic_texture_paths;
    TextureWrap metallic_texture_wrap;
    std::vector<std::filesystem::path> roughness_texture_paths;
    TextureWrap roughness_texture_wrap;
    std::vector<std::filesystem::path> ambient_occlusion_texture_paths;
    TextureWrap ambient_occlusion_texture_wrap;

    AlphaMode alpha_mode = AlphaMode::Opaque;
};

struct ModelData {
    std::vector<MeshData> meshes;
    std::unordered_map<std::filesystem::path, ImageLoader::Image> textures_map;
};

auto load_model(const std::filesystem::path& path) -> std::optional<ModelData>;

}  // namespace Luminol::Utilities::ModelLoader
