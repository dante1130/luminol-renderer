#pragma once

#include <filesystem>

#include <LuminolRenderEngine/Utilities/ImageLoader.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLUniformBindingPoints.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLTextureFormat.hpp>

namespace Luminol::Graphics {

enum class ColorSpace { SRGB, Linear };

class OpenGLTexture {
public:
    OpenGLTexture(
        int32_t width, int32_t height, TextureInternalFormat internal_format
    );

    OpenGLTexture(const std::filesystem::path& path, ColorSpace color_space);

    OpenGLTexture(
        const Utilities::ImageLoader::Image& image, ColorSpace color_space
    );

    ~OpenGLTexture();
    OpenGLTexture(const OpenGLTexture&) = default;
    OpenGLTexture(OpenGLTexture&&) = delete;
    auto operator=(const OpenGLTexture&) -> OpenGLTexture& = default;
    auto operator=(OpenGLTexture&&) -> OpenGLTexture& = delete;

    auto bind(SamplerBindingPoint binding_point) const -> void;
    auto unbind(SamplerBindingPoint binding_point) const -> void;

    auto bind_image(ImageBindingPoint binding_point, ImageAccess access) const
        -> void;

private:
    uint32_t texture_id = {0};
    TextureInternalFormat internal_format = {TextureInternalFormat::RGBA8};
};

}  // namespace Luminol::Graphics
