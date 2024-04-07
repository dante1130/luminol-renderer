#pragma once

#include <cstdint>

#include <glm/glm.hpp>

#include <Engine/Graphics/OpenGL/OpenGLUniformBindingPoints.hpp>

namespace Luminol::Graphics {

class OpenGLFrameBuffer {
public:
    OpenGLFrameBuffer(int32_t width, int32_t height);
    ~OpenGLFrameBuffer();
    OpenGLFrameBuffer(const OpenGLFrameBuffer&) = delete;
    OpenGLFrameBuffer(OpenGLFrameBuffer&&) = default;
    auto operator=(const OpenGLFrameBuffer&) -> OpenGLFrameBuffer& = delete;
    auto operator=(OpenGLFrameBuffer&&) -> OpenGLFrameBuffer& = default;

    auto bind() const -> void;
    auto unbind() const -> void;

    auto bind_color_attachment(SamplerBindingPoint binding_point) const -> void;
    auto unbind_color_attachment(SamplerBindingPoint binding_point) const
        -> void;

    auto blit(int32_t width, int32_t height) const -> void;

    [[nodiscard]] auto get_width() const -> int32_t;
    [[nodiscard]] auto get_height() const -> int32_t;

    auto resize(int32_t width, int32_t height) -> void;

private:
    int32_t width = {0};
    int32_t height = {0};

    uint32_t frame_buffer_id = {0};
    uint32_t color_attachment_id = {0};
    uint32_t render_buffer_id = {0};
};

}  // namespace Luminol::Graphics
