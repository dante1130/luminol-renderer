#include "OpenGLTexture.hpp"

#include <glad/gl.h>

#include <Engine/Utilities/ImageLoader.hpp>

namespace Luminol::Graphics {

OpenGLTexture::OpenGLTexture(const std::filesystem::path& path) {
    glCreateTextures(GL_TEXTURE_2D, 1, &this->texture_id);

    const auto image = Utilities::ImageLoader::load_image(path);

    glTextureParameteri(texture_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(texture_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(texture_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(texture_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTextureStorage2D(
        texture_id,
        1,
        image.channels == 4 ? GL_RGBA8 : GL_RGB8,
        image.width,
        image.height
    );

    glTextureSubImage2D(
        texture_id,
        0,
        0,
        0,
        image.width,
        image.height,
        image.channels == 4 ? GL_RGBA : GL_RGB,
        GL_UNSIGNED_BYTE,
        image.data.data()
    );

    glGenerateTextureMipmap(texture_id);
}

OpenGLTexture::~OpenGLTexture() { glDeleteTextures(1, &texture_id); }

auto OpenGLTexture::bind() const -> void { glBindTextureUnit(0, texture_id); }

// NOLINTBEGIN(readability-convert-member-functions-to-static)
auto OpenGLTexture::unbind() const -> void { glBindTextureUnit(0, 0); }
// NOLINTEND(readability-convert-member-functions-to-static)

}  // namespace Luminol::Graphics
