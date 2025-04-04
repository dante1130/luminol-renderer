#pragma once

#include <cstdint>

namespace Luminol::Graphics {

enum class TextureInternalFormat : uint8_t {
    R8,
    RG8,
    RGB8,
    RGBA8,
    R16F,
    RG16F,
    RGB16F,
    RGBA16F,
    R32F,
    RG32F,
    RGB32F,
    RGBA32F,
    SRGB8,
    SRGB8_Alpha8,
};

enum class TextureFormat : uint8_t {
    Red,
    RG,
    RGB,
    RGBA,
};

enum class ImageAccess : uint8_t {
    Read,
    Write,
    ReadWrite,
};

auto get_opengl_internal_format(TextureInternalFormat format) -> int32_t;
auto get_opengl_format(TextureFormat format) -> int32_t;

auto get_internal_format_from_channels(int32_t channels)
    -> TextureInternalFormat;
auto get_internal_format_from_channels_srgb(int32_t channels)
    -> TextureInternalFormat;
auto get_format_from_channels(int32_t channels) -> TextureFormat;

auto get_opengl_data_type_from_internal_format(TextureInternalFormat format)
    -> int32_t;

auto get_opengl_format_from_internal_format(TextureInternalFormat format)
    -> int32_t;

auto get_opengl_image_access(ImageAccess access) -> int32_t;

}  // namespace Luminol::Graphics
