#include "OpenGLFrameBuffer.hpp"

#include <gsl/gsl>
#include <glad/gl.h>

#include <Engine/Graphics/OpenGL/OpenGLBufferBit.hpp>

namespace Luminol::Graphics {

OpenGLFrameBuffer::OpenGLFrameBuffer(
    const OpenGLFrameBufferDescriptor& descriptor
)
    : width(descriptor.width),
      height(descriptor.height),
      color_attachments(descriptor.color_attachments) {
    glCreateFramebuffers(1, &this->frame_buffer_id);

    auto draw_buffers = std::vector<GLenum>{};
    draw_buffers.reserve(this->color_attachments.size());

    for (size_t i = 0; i < color_attachments.size(); ++i) {
        auto& color_attachment = color_attachments[i];

        glCreateTextures(GL_TEXTURE_2D, 1, &color_attachment.attachment_id);
        glTextureImage2DEXT(
            color_attachment.attachment_id,
            GL_TEXTURE_2D,
            0,
            get_opengl_internal_format(color_attachment.internal_format),
            width,
            height,
            0,
            get_opengl_format(color_attachment.format),
            get_opengl_data_type_from_internal_format(
                color_attachment.internal_format
            ),
            nullptr
        );
        glTextureParameteri(
            color_attachment.attachment_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR
        );
        glTextureParameteri(
            color_attachment.attachment_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR
        );
        glTextureParameteri(
            color_attachment.attachment_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE
        );
        glTextureParameteri(
            color_attachment.attachment_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE
        );

        const auto color_attachment_point =
            GL_COLOR_ATTACHMENT0 + gsl::narrow_cast<GLenum>(i);

        glNamedFramebufferTexture(
            this->frame_buffer_id,
            color_attachment_point,
            color_attachment.attachment_id,
            0
        );

        draw_buffers.push_back(color_attachment_point);
    }

    glNamedFramebufferDrawBuffers(
        this->frame_buffer_id,
        gsl::narrow_cast<GLsizei>(draw_buffers.size()),
        draw_buffers.data()
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
    for (const auto& color_attachment : this->color_attachments) {
        glDeleteTextures(1, &color_attachment.attachment_id);
    }
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

auto OpenGLFrameBuffer::bind_color_attachments() const -> void {
    for (const auto& color_attachment : this->color_attachments) {
        glBindTextureUnit(
            static_cast<uint32_t>(color_attachment.binding_point),
            color_attachment.attachment_id
        );
    }
}

auto OpenGLFrameBuffer::unbind_color_attachments() const -> void {
    for (const auto& color_attachment : this->color_attachments) {
        glBindTextureUnit(
            static_cast<uint32_t>(color_attachment.binding_point), 0
        );
    }
}

auto OpenGLFrameBuffer::blit_to_default_framebuffer(
    int32_t width, int32_t height, BufferBit buffer_bit
) const -> void {
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
        buffer_bit_to_gl(buffer_bit),
        GL_NEAREST
    );
}

auto OpenGLFrameBuffer::blit_to_framebuffer(
    const OpenGLFrameBuffer& frame_buffer, BufferBit buffer_bit
) const -> void {
    glBlitNamedFramebuffer(
        this->frame_buffer_id,
        frame_buffer.frame_buffer_id,
        0,
        0,
        this->width,
        this->height,
        0,
        0,
        frame_buffer.get_width(),
        frame_buffer.get_height(),
        buffer_bit_to_gl(buffer_bit),
        GL_NEAREST
    );
}

auto OpenGLFrameBuffer::get_frame_buffer_id() const -> uint32_t {
    return this->frame_buffer_id;
}

auto OpenGLFrameBuffer::get_width() const -> int32_t { return this->width; }

auto OpenGLFrameBuffer::get_height() const -> int32_t { return this->height; }

auto OpenGLFrameBuffer::resize(int32_t width, int32_t height) -> void {
    this->width = width;
    this->height = height;

    for (const auto& color_attachment : this->color_attachments) {
        glTextureImage2DEXT(
            color_attachment.attachment_id,
            GL_TEXTURE_2D,
            0,
            get_opengl_internal_format(color_attachment.internal_format),
            width,
            height,
            0,
            get_opengl_format(color_attachment.format),
            get_opengl_data_type_from_internal_format(
                color_attachment.internal_format
            ),
            nullptr
        );
    }

    glNamedRenderbufferStorageEXT(
        this->render_buffer_id, GL_DEPTH24_STENCIL8, this->width, this->height
    );
}

}  // namespace Luminol::Graphics
