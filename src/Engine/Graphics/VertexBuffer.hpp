#pragma once

#include <gsl/gsl>

namespace Luminol::Graphics {

class VertexBuffer {
public:
    VertexBuffer(gsl::span<const float> vertices);
    virtual ~VertexBuffer() = default;
    VertexBuffer(const VertexBuffer&) = default;
    VertexBuffer(VertexBuffer&&) = delete;
    auto operator=(const VertexBuffer&) -> VertexBuffer& = default;
    auto operator=(VertexBuffer&&) -> VertexBuffer& = delete;

    virtual auto bind() const -> void = 0;
    virtual auto unbind() const -> void = 0;
};

}  // namespace Luminol::Graphics
