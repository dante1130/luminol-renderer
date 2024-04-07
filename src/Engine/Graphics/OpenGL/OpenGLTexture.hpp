#pragma once

#include <filesystem>

#include <Engine/Utilities/ImageLoader.hpp>
#include <Engine/Graphics/OpenGL/OpenGLUniformBindingPoints.hpp>

namespace Luminol::Graphics {

enum class ColorSpace { SRGB, Linear };

class OpenGLTexture {
public:
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

private:
    uint32_t texture_id = {0};
};

}  // namespace Luminol::Graphics
