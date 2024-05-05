#include "OpenGLSkybox.hpp"

#include <gsl/gsl>
#include <glad/gl.h>

#include <LuminolRenderEngine/Utilities/ImageLoader.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLUniformBindingPoints.hpp>

namespace {

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

auto load_skybox_face(
    uint32_t skybox_texture_id, const std::filesystem::path& path, GLenum face
) {
    auto image = Luminol::Utilities::ImageLoader::load_image(path);

    const auto internal_format =
        channels_to_internal_format_srgb(image.channels);
    const auto format = channels_to_format(image.channels);

    glTextureImage2DEXT(
        skybox_texture_id,
        face,
        0,
        gsl::narrow<int32_t>(internal_format),
        image.width,
        image.height,
        0,
        format,
        GL_UNSIGNED_BYTE,
        image.data.data()
    );
}

}  // namespace

namespace Luminol::Graphics {

OpenGLSkybox::OpenGLSkybox(const SkyboxPaths& paths) {
    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &skybox_texture_id);

    load_skybox_face(
        skybox_texture_id, paths.front, GL_TEXTURE_CUBE_MAP_POSITIVE_Z
    );
    load_skybox_face(
        skybox_texture_id, paths.back, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
    );
    load_skybox_face(
        skybox_texture_id, paths.top, GL_TEXTURE_CUBE_MAP_POSITIVE_Y
    );
    load_skybox_face(
        skybox_texture_id, paths.bottom, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y
    );
    load_skybox_face(
        skybox_texture_id, paths.left, GL_TEXTURE_CUBE_MAP_NEGATIVE_X
    );
    load_skybox_face(
        skybox_texture_id, paths.right, GL_TEXTURE_CUBE_MAP_POSITIVE_X
    );

    glTextureParameteri(skybox_texture_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(skybox_texture_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(skybox_texture_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(skybox_texture_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(skybox_texture_id, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glGenerateTextureMipmap(skybox_texture_id);
}

auto OpenGLSkybox::bind() const -> void {
    glBindTextureUnit(
        static_cast<uint32_t>(SamplerBindingPoint::Skybox), skybox_texture_id
    );
}

// NOLINTBEGIN(readability-convert-member-functions-to-static)
auto OpenGLSkybox::unbind() const -> void {
    glBindTextureUnit(static_cast<uint32_t>(SamplerBindingPoint::Skybox), 0);
}
// NOLINTEND(readability-convert-member-functions-to-static)

}  // namespace Luminol::Graphics
