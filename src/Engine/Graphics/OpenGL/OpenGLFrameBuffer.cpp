#include "OpenGLFrameBuffer.hpp"

#include <gsl/gsl>
#include <glad/gl.h>

namespace Luminol::Graphics {

OpenGLFrameBuffer::OpenGLFrameBuffer(int32_t width, int32_t height)
    : width(width), height(height) {
    glCreateFramebuffers(1, &this->frame_buffer_id);

    glCreateTextures(GL_TEXTURE_2D, 1, &this->color_attachment_id);
    glTextureImage2DEXT(
        this->color_attachment_id,
        GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        this->width,
        this->height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        nullptr
    );
    glNamedFramebufferTexture(
        this->frame_buffer_id,
        GL_COLOR_ATTACHMENT0,
        this->color_attachment_id,
        0
    );

    glCreateRenderbuffers(1, &this->render_buffer_id);
    glNamedRenderbufferStorageEXT(
        this->render_buffer_id, GL_DEPTH24_STENCIL8, this->width, this->height
    );
    glNamedFramebufferRenderbuffer(
        this->frame_buffer_id,
        GL_DEPTH_STENCIL_ATTACHMENT,
        GL_RENDERBUFFER,
        this->render_buffer_id
    );

    Expects(
        glCheckNamedFramebufferStatus(this->frame_buffer_id, GL_FRAMEBUFFER) ==
        GL_FRAMEBUFFER_COMPLETE
    );
}

OpenGLFrameBuffer::~OpenGLFrameBuffer() {
    glDeleteFramebuffers(1, &this->frame_buffer_id);
    glDeleteTextures(1, &this->color_attachment_id);
    glDeleteRenderbuffers(1, &this->render_buffer_id);
}

auto OpenGLFrameBuffer::bind() const -> void {
    glBindFramebuffer(GL_FRAMEBUFFER, this->frame_buffer_id);
}

// NOLINTBEGIN(readability-convert-member-functions-to-static)
auto OpenGLFrameBuffer::unbind() const -> void {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
// NOLINTEND(readability-convert-member-functions-to-static)

auto OpenGLFrameBuffer::blit(int32_t width, int32_t height) const -> void {
    glBlitNamedFramebuffer(
        this->frame_buffer_id,
        0,
        0,
        0,
        this->width,
        this->height,
        0,
        0,
        width,
        height,
        GL_COLOR_BUFFER_BIT,
        GL_NEAREST
    );
}

auto OpenGLFrameBuffer::get_width() const -> int32_t { return this->width; }

auto OpenGLFrameBuffer::get_height() const -> int32_t { return this->height; }

auto OpenGLFrameBuffer::resize(int32_t width, int32_t height) -> void {
    this->width = width;
    this->height = height;

    glTextureImage2DEXT(
        this->color_attachment_id,
        GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        this->width,
        this->height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        nullptr
    );

    glNamedRenderbufferStorageEXT(
        this->render_buffer_id, GL_DEPTH24_STENCIL8, this->width, this->height
    );
}

}  // namespace Luminol::Graphics
