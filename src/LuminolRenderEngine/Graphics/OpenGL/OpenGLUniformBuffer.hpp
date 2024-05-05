#pragma once

#include <cstdint>

#include <glad/gl.h>

#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLUniformBindingPoints.hpp>

namespace Luminol::Graphics {

template <typename T>
class OpenGLUniformBuffer {
public:
    OpenGLUniformBuffer(
        const T& data, UniformBufferBindingPoint binding_point
    ) {
        glCreateBuffers(1, &uniform_buffer_id);
        glNamedBufferData(uniform_buffer_id, sizeof(T), &data, GL_DYNAMIC_DRAW);
        glBindBufferRange(
            GL_UNIFORM_BUFFER,
            static_cast<uint32_t>(binding_point),
            uniform_buffer_id,
            0,
            sizeof(T)
        );
    }

    ~OpenGLUniformBuffer() { glDeleteBuffers(1, &uniform_buffer_id); }

    OpenGLUniformBuffer(const OpenGLUniformBuffer&) = delete;
    OpenGLUniformBuffer(OpenGLUniformBuffer&&) = default;
    auto operator=(const OpenGLUniformBuffer&) -> OpenGLUniformBuffer& = delete;
    auto operator=(OpenGLUniformBuffer&&) -> OpenGLUniformBuffer& = default;

    auto set_data(const T& data) -> void {
        glNamedBufferSubData(uniform_buffer_id, 0, sizeof(T), &data);
    }

private:
    uint32_t uniform_buffer_id = 0;
};

}  // namespace Luminol::Graphics
