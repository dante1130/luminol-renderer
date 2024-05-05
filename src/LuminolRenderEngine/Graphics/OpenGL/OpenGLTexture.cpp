#include "OpenGLTexture.hpp"

#include <glad/gl.h>

#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLTextureFormat.hpp>

namespace {

using namespace Luminol::Graphics;

auto create_texture(
    const Luminol::Utilities::ImageLoader::Image& image, ColorSpace color_space
) -> uint32_t {
    uint32_t texture_id = {0};
    glCreateTextures(GL_TEXTURE_2D, 1, &texture_id);

    glTextureParameteri(texture_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(texture_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(texture_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(texture_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    const auto internal_format =
        color_space == ColorSpace::SRGB
            ? get_internal_format_from_channels_srgb(image.channels)
            : get_internal_format_from_channels(image.channels);

    glTextureStorage2D(
        texture_id,
        1,
        get_opengl_internal_format(internal_format),
        image.width,
        image.height
    );

    const auto format = get_format_from_channels(image.channels);

    glTextureSubImage2D(
        texture_id,
        0,
        0,
        0,
        image.width,
        image.height,
        get_opengl_format(format),
        get_opengl_data_type_from_internal_format(internal_format),
        image.data.data()
    );

    glGenerateTextureMipmap(texture_id);

    return texture_id;
}

}  // namespace

namespace Luminol::Graphics {

OpenGLTexture::OpenGLTexture(
    const std::filesystem::path& path, ColorSpace color_space
)
    : texture_id(
          create_texture(Utilities::ImageLoader::load_image(path), color_space)
      ) {}

OpenGLTexture::OpenGLTexture(
    const Utilities::ImageLoader::Image& image, ColorSpace color_space
)
    : texture_id(create_texture(image, color_space)) {}

OpenGLTexture::~OpenGLTexture() { glDeleteTextures(1, &this->texture_id); }

auto OpenGLTexture::bind(SamplerBindingPoint binding_point) const -> void {
    glBindTextureUnit(static_cast<uint32_t>(binding_point), this->texture_id);
}

// NOLINTBEGIN(readability-convert-member-functions-to-static)
auto OpenGLTexture::unbind(SamplerBindingPoint binding_point) const -> void {
    glBindTextureUnit(static_cast<uint32_t>(binding_point), 0);
}
// NOLINTEND(readability-convert-member-functions-to-static)

}  // namespace Luminol::Graphics
