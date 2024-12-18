#pragma once

#include <unordered_map>

#include <LuminolRenderEngine/Utilities/ImageLoader.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLTexture.hpp>

namespace Luminol::Graphics {

class OpenGLTextureManager {
public:
    using TextureMap = std::unordered_map<std::filesystem::path, OpenGLTexture>;

    static auto get_instance() -> OpenGLTextureManager&;

    auto load_texture(const std::filesystem::path& path, ColorSpace color_space)
        -> void;
    auto load_texture(
        const Utilities::ImageLoader::Image& image, ColorSpace color_space
    ) -> void;

    [[nodiscard]] auto get_texture(const std::filesystem::path& path) const
        -> const OpenGLTexture&;

private:
    OpenGLTextureManager();
    TextureMap texture_map;
};

}  // namespace Luminol::Graphics
