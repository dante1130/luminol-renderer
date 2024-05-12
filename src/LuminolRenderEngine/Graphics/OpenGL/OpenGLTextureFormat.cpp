#include "OpenGLTextureFormat.hpp"

#include <stdexcept>

#include <glad/gl.h>

namespace Luminol::Graphics {

auto get_opengl_internal_format(TextureInternalFormat internal_format)
    -> int32_t {
    switch (internal_format) {
        case TextureInternalFormat::R8:
            return GL_R8;
        case TextureInternalFormat::RG8:
            return GL_RG8;
        case TextureInternalFormat::RGB8:
            return GL_RGB8;
        case TextureInternalFormat::RGBA8:
            return GL_RGBA8;
        case TextureInternalFormat::R16F:
            return GL_R16F;
        case TextureInternalFormat::RG16F:
            return GL_RG16F;
        case TextureInternalFormat::RGB16F:
            return GL_RGB16F;
        case TextureInternalFormat::RGBA16F:
            return GL_RGBA16F;
        case TextureInternalFormat::R32F:
            return GL_R32F;
        case TextureInternalFormat::RG32F:
            return GL_RG32F;
        case TextureInternalFormat::RGB32F:
            return GL_RGB32F;
        case TextureInternalFormat::RGBA32F:
            return GL_RGBA32F;
        case TextureInternalFormat::SRGB8:
            return GL_SRGB8;
        case TextureInternalFormat::SRGB8_Alpha8:
            return GL_SRGB8_ALPHA8;
        default:
            throw std::runtime_error("Invalid internal format");
    }
}

auto get_opengl_format(TextureFormat format) -> int32_t {
    switch (format) {
        case TextureFormat::Red:
            return GL_RED;
        case TextureFormat::RG:
            return GL_RG;
        case TextureFormat::RGB:
            return GL_RGB;
        case TextureFormat::RGBA:
            return GL_RGBA;
        default:
            throw std::runtime_error("Invalid format");
    }
}

auto get_internal_format_from_channels(int32_t channels)
    -> TextureInternalFormat {
    switch (channels) {
        case 1:
            return TextureInternalFormat::R8;
        case 2:
            return TextureInternalFormat::RG8;
        case 3:
            return TextureInternalFormat::RGB8;
        case 4:
            return TextureInternalFormat::RGBA8;
        default:
            throw std::runtime_error("Invalid number of channels");
    }
}

auto get_internal_format_from_channels_srgb(int32_t channels)
    -> TextureInternalFormat {
    switch (channels) {
        case 1:
            return TextureInternalFormat::SRGB8;
        case 2:
            return TextureInternalFormat::SRGB8_Alpha8;
        case 3:
            return TextureInternalFormat::SRGB8;
        case 4:
            return TextureInternalFormat::SRGB8_Alpha8;
        default:
            throw std::runtime_error("Invalid number of channels");
    }
}

auto get_format_from_channels(int32_t channels) -> TextureFormat {
    switch (channels) {
        case 1:
            return TextureFormat::Red;
        case 2:
            return TextureFormat::RG;
        case 3:
            return TextureFormat::RGB;
        case 4:
            return TextureFormat::RGBA;
        default:
            throw std::runtime_error("Invalid number of channels");
    }
}

auto get_opengl_data_type_from_internal_format(TextureInternalFormat format)
    -> int32_t {
    switch (format) {
        case TextureInternalFormat::R8:
        case TextureInternalFormat::RG8:
        case TextureInternalFormat::RGB8:
        case TextureInternalFormat::RGBA8:
            return GL_UNSIGNED_BYTE;
        case TextureInternalFormat::R16F:
        case TextureInternalFormat::RG16F:
        case TextureInternalFormat::RGB16F:
        case TextureInternalFormat::RGBA16F:
            return GL_HALF_FLOAT;
        case TextureInternalFormat::R32F:
        case TextureInternalFormat::RG32F:
        case TextureInternalFormat::RGB32F:
        case TextureInternalFormat::RGBA32F:
            return GL_FLOAT;
        case TextureInternalFormat::SRGB8:
        case TextureInternalFormat::SRGB8_Alpha8:
            return GL_UNSIGNED_BYTE;
        default:
            throw std::runtime_error("Invalid internal format");
    }
}

auto get_opengl_image_access(ImageAccess access) -> int32_t {
    switch (access) {
        case ImageAccess::Read:
            return GL_READ_ONLY;
        case ImageAccess::Write:
            return GL_WRITE_ONLY;
        case ImageAccess::ReadWrite:
            return GL_READ_WRITE;
        default:
            throw std::runtime_error("Invalid access");
    }
}

}  // namespace Luminol::Graphics
