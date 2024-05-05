#pragma once

#include <gsl/gsl>

namespace Luminol::Graphics {

class OpenGLIndexBuffer {
public:
    OpenGLIndexBuffer(gsl::span<const uint32_t> indices);
    ~OpenGLIndexBuffer();
    OpenGLIndexBuffer(const OpenGLIndexBuffer&) = delete;
    OpenGLIndexBuffer(OpenGLIndexBuffer&&) = default;
    auto operator=(const OpenGLIndexBuffer&) -> OpenGLIndexBuffer& = delete;
    auto operator=(OpenGLIndexBuffer&&) -> OpenGLIndexBuffer& = default;

    [[nodiscard]] auto get_index_buffer_id() const -> uint32_t;
    [[nodiscard]] auto get_index_count() const -> int32_t;

private:
    uint32_t index_buffer_id = 0;
    int32_t index_count = 0;
};

}  // namespace Luminol::Graphics
