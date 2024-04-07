#include "OpenGLTexture.hpp"

#include <glad/gl.h>
#include "Engine/Graphics/OpenGL/OpenGLUniformBindingPoints.hpp"

namespace {

using namespace Luminol::Graphics;

constexpr auto channels_to_internal_format_linear(int32_t channels) -> GLenum {
    switch (channels) {
        case 1:
            return GL_R8;
        case 2:
            return GL_RG8;
        case 3:
            return GL_RGB8;
        case 4:
            return GL_RGBA8;
        default:
            throw std::runtime_error{"Invalid number of channels"};
    }
}

constexpr auto channels_to_internal_format_srgb(int32_t channels) -> GLenum {
    switch (channels) {
        case 1:
            return GL_SRGB8;
        case 2:
            return GL_SRGB8_ALPHA8;
        case 3:
            return GL_SRGB8;
        case 4:
            return GL_SRGB8_ALPHA8;
        default:
            throw std::runtime_error{"Invalid number of channels"};
    }
}

constexpr auto channels_to_format(int32_t channels) -> GLenum {
    switch (channels) {
        case 1:
            return GL_RED;
        case 2:
            return GL_RG;
        case 3:
            return GL_RGB;
        case 4:
            return GL_RGBA;
        default:
            throw std::runtime_error{"Invalid number of channels"};
    }
}

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
            ? channels_to_internal_format_srgb(image.channels)
            : channels_to_internal_format_linear(image.channels);

    glTextureStorage2D(
        texture_id, 1, internal_format, image.width, image.height
    );

    glTextureSubImage2D(
        texture_id,
        0,
        0,
        0,
        image.width,
        image.height,
        channels_to_format(image.channels),
        GL_UNSIGNED_BYTE,
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
