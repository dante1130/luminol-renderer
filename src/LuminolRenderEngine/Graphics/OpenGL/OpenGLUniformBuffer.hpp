#pragma once

#include <cstdint>

#include <glad/gl.h>

#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLUniformBindingPoints.hpp>

namespace Luminol::Graphics {

class OpenGLUniformBuffer {
public:
    OpenGLUniformBuffer(UniformBufferBindingPoint binding_point)
        : binding_point{binding_point} {
        glCreateBuffers(1, &uniform_buffer_id);
        glBindBufferBase(
            GL_UNIFORM_BUFFER,
            static_cast<uint32_t>(this->binding_point),
            this->uniform_buffer_id
        );
    }

    ~OpenGLUniformBuffer() { glDeleteBuffers(1, &uniform_buffer_id); }

    OpenGLUniformBuffer(const OpenGLUniformBuffer&) = delete;
    OpenGLUniformBuffer(OpenGLUniformBuffer&&) = default;
    auto operator=(const OpenGLUniformBuffer&) -> OpenGLUniformBuffer& = delete;
    auto operator=(OpenGLUniformBuffer&&) -> OpenGLUniformBuffer& = default;

    template <typename T>
    auto set_data(int64_t offset, int64_t data_size_bytes, const T* data)
        -> void {
        const auto new_size_bytes = offset + data_size_bytes;

        if (this->capacity_bytes < new_size_bytes) {
            uint32_t new_uniform_buffer_id = 0;
            glCreateBuffers(1, &new_uniform_buffer_id);
            glNamedBufferData(
                new_uniform_buffer_id, new_size_bytes, nullptr, GL_DYNAMIC_DRAW
            );
            glBindBufferBase(
                GL_UNIFORM_BUFFER,
                static_cast<uint32_t>(this->binding_point),
                new_uniform_buffer_id
            );

            glCopyNamedBufferSubData(
                this->uniform_buffer_id,
                new_uniform_buffer_id,
                0,
                0,
                this->capacity_bytes
            );

            glDeleteBuffers(1, &this->uniform_buffer_id);

            this->uniform_buffer_id = new_uniform_buffer_id;
            this->capacity_bytes = new_size_bytes;
        }

        glNamedBufferSubData(
            this->uniform_buffer_id, offset, data_size_bytes, data
        );
    }

private:
    UniformBufferBindingPoint binding_point;
    uint32_t uniform_buffer_id = 0;
    int64_t capacity_bytes = 0;
};

}  // namespace Luminol::Graphics
