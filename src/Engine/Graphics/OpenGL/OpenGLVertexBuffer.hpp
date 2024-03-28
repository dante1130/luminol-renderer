#pragma once

#include <Engine/Graphics/VertexBuffer.hpp>

namespace Luminol::Graphics {

class OpenGLVertexBuffer : public VertexBuffer {
public:
    OpenGLVertexBuffer(gsl::span<const float> vertices);
    ~OpenGLVertexBuffer() override;
    OpenGLVertexBuffer(const OpenGLVertexBuffer&) = default;
    OpenGLVertexBuffer(OpenGLVertexBuffer&&) = delete;
    auto operator=(const OpenGLVertexBuffer&) -> OpenGLVertexBuffer& = default;
    auto operator=(OpenGLVertexBuffer&&) -> OpenGLVertexBuffer& = delete;

    auto bind() const -> void override;
    auto unbind() const -> void override;

private:
    uint32_t vertex_buffer_id = {0};
};

}  // namespace Luminol::Graphics
