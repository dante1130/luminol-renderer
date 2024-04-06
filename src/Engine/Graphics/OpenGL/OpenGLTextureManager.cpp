#include "OpenGLTextureManager.hpp"

namespace {

constexpr auto default_texture_path = "res/textures/reflex.png";

}

namespace Luminol::Graphics {

OpenGLTextureManager::OpenGLTextureManager() {
    this->load_texture(default_texture_path);
}

auto OpenGLTextureManager::get_instance() -> OpenGLTextureManager& {
    static auto instance = OpenGLTextureManager{};
    return instance;
}

auto OpenGLTextureManager::load_texture(const std::filesystem::path& path)
    -> void {
    if (this->texture_map.contains(path)) {
        return;
    }

    this->texture_map.emplace(path, path);
}

auto OpenGLTextureManager::load_texture(
    const Utilities::ImageLoader::Image& image
) -> void {
    if (this->texture_map.contains(image.path)) {
        return;
    }

    this->texture_map.emplace(image.path, image);
}

auto OpenGLTextureManager::get_texture(const std::filesystem::path& path) const
    -> const OpenGLTexture& {
    return this->texture_map.contains(path)
               ? this->texture_map.at(path)
               : this->texture_map.at(default_texture_path);
}

}  // namespace Luminol::Graphics
