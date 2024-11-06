#pragma once

#include <cstdint>
#include <vector> 

#include <glm/glm.hpp>

#include <LuminolRenderEngine/Graphics/BufferBit.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLTextureFormat.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLUniformBindingPoints.hpp>

namespace Luminol::Graphics {

struct OpenGLFrameBufferAttachment {
    uint32_t attachment_id = {0};
    TextureInternalFormat internal_format = {TextureInternalFormat::RGBA8};
    TextureFormat format = {TextureFormat::RGBA};
    SamplerBindingPoint binding_point = {SamplerBindingPoint::HDRFramebuffer};
};

struct OpenGLFrameBufferDescriptor {
    int32_t width = {0};
    int32_t height = {0};
    std::vector<OpenGLFrameBufferAttachment> color_attachments = {};
};

class OpenGLFrameBuffer {
public:
    OpenGLFrameBuffer(const OpenGLFrameBufferDescriptor& descriptor);
    ~OpenGLFrameBuffer();
    OpenGLFrameBuffer(const OpenGLFrameBuffer&) = delete;
    OpenGLFrameBuffer(OpenGLFrameBuffer&&) = default;
    auto operator=(const OpenGLFrameBuffer&) -> OpenGLFrameBuffer& = delete;
    auto operator=(OpenGLFrameBuffer&&) -> OpenGLFrameBuffer& = default;

    auto bind() const -> void;
    auto unbind() const -> void;

    auto bind_color_attachments() const -> void;
    auto unbind_color_attachments() const -> void;

    auto bind_image(ImageBindingPoint binding_point, ImageAccess access) const
        -> void;

    auto blit_to_default_framebuffer(
        int32_t width, int32_t height, BufferBit buffer_bit
    ) const -> void;
    auto blit_to_framebuffer(
        const OpenGLFrameBuffer& frame_buffer, BufferBit buffer_bit
    ) const -> void;

    [[nodiscard]] auto get_frame_buffer_id() const -> uint32_t;
    [[nodiscard]] auto get_width() const -> int32_t;
    [[nodiscard]] auto get_height() const -> int32_t;

    auto resize(int32_t width, int32_t height) -> void;

private:
    int32_t width = {0};
    int32_t height = {0};

    uint32_t frame_buffer_id = {0};
    std::vector<OpenGLFrameBufferAttachment> color_attachments = {};
    uint32_t render_buffer_id = {0};
};

}  // namespace Luminol::Graphics
