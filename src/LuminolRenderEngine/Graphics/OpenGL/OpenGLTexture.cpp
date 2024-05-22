#include "OpenGLTexture.hpp"

#include <glad/gl.h>

namespace {

using namespace Luminol::Graphics;

struct TextureData {
    uint32_t texture_id = {0};
    TextureInternalFormat internal_format = {TextureInternalFormat::RGBA8};
};

auto create_texture(
    const Luminol::Utilities::ImageLoader::Image& image, ColorSpace color_space
) -> TextureData {
    uint32_t texture_id = {0};
    glCreateTextures(GL_TEXTURE_2D, 1, &texture_id);

    glTextureParameteri(texture_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(texture_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(texture_id, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(texture_id, GL_TEXTURE_WRAP_T, GL_REPEAT);

    const auto internal_format =
        color_space == ColorSpace::SRGB
            ? get_internal_format_from_channels_srgb(image.channels)
            : get_internal_format_from_channels(image.channels);

    const auto format = get_format_from_channels(image.channels);

    glTextureImage2DEXT(
        texture_id,
        GL_TEXTURE_2D,
        0,
        get_opengl_internal_format(internal_format),
        image.width,
        image.height,
        0,
        get_opengl_format(format),
        get_opengl_data_type_from_internal_format(internal_format),
        image.data.data()
    );

    glGenerateTextureMipmap(texture_id);

    return TextureData{texture_id, internal_format};
}

}  // namespace

namespace Luminol::Graphics {

OpenGLTexture::OpenGLTexture(
    int32_t width, int32_t height, TextureInternalFormat internal_format
)
    : internal_format(internal_format) {
    glCreateTextures(GL_TEXTURE_2D, 1, &this->texture_id);

    glTextureParameteri(this->texture_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(this->texture_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(this->texture_id, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(this->texture_id, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTextureImage2DEXT(
        this->texture_id,
        GL_TEXTURE_2D,
        0,
        get_opengl_internal_format(internal_format),
        width,
        height,
        0,
        get_opengl_format_from_internal_format(internal_format),
        get_opengl_data_type_from_internal_format(internal_format),
        nullptr
    );
}

OpenGLTexture::OpenGLTexture(
    const std::filesystem::path& path, ColorSpace color_space
) {
    const auto texture_data = create_texture(
        Luminol::Utilities::ImageLoader::load_image(path), color_space
    );

    this->texture_id = texture_data.texture_id;
    this->internal_format = texture_data.internal_format;
}

OpenGLTexture::OpenGLTexture(
    const Utilities::ImageLoader::Image& image, ColorSpace color_space
) {
    const auto texture_data = create_texture(image, color_space);

    this->texture_id = texture_data.texture_id;
    this->internal_format = texture_data.internal_format;
}

OpenGLTexture::~OpenGLTexture() { glDeleteTextures(1, &this->texture_id); }

auto OpenGLTexture::bind(SamplerBindingPoint binding_point) const -> void {
    glBindTextureUnit(static_cast<uint32_t>(binding_point), this->texture_id);
}

// NOLINTBEGIN(readability-convert-member-functions-to-static)
auto OpenGLTexture::unbind(SamplerBindingPoint binding_point) const -> void {
    glBindTextureUnit(static_cast<uint32_t>(binding_point), 0);
}
// NOLINTEND(readability-convert-member-functions-to-static)

auto OpenGLTexture::bind_image(
    ImageBindingPoint binding_point, ImageAccess access
) const -> void {
    glBindImageTexture(
        static_cast<uint32_t>(binding_point),
        this->texture_id,
        0,
        GL_FALSE,
        0,
        get_opengl_image_access(access),
        get_opengl_internal_format(this->internal_format)
    );
}

auto OpenGLTexture::resize(int32_t width, int32_t height) -> void {
    glTextureImage2DEXT(
        this->texture_id,
        GL_TEXTURE_2D,
        0,
        get_opengl_internal_format(this->internal_format),
        width,
        height,
        0,
        get_opengl_format_from_internal_format(this->internal_format),
        get_opengl_data_type_from_internal_format(this->internal_format),
        nullptr
    );
}

}  // namespace Luminol::Graphics
