#pragma once

#include <cstdint>

#include <glad/gl.h>
#include <gsl/gsl>

#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLUniformBindingPoints.hpp>

namespace Luminol::Graphics {

class OpenGLShaderStorageBuffer {
public:
    OpenGLShaderStorageBuffer(ShaderStorageBufferBindingPoint binding_point)
        : binding_point{binding_point} {
        glCreateBuffers(1, &this->shader_storage_buffer_id);
        glBindBufferBase(
            GL_SHADER_STORAGE_BUFFER,
            static_cast<uint32_t>(this->binding_point),
            this->shader_storage_buffer_id
        );
    }

    ~OpenGLShaderStorageBuffer() {
        glDeleteBuffers(1, &this->shader_storage_buffer_id);
    }

    template <typename T>
    auto set_data(int64_t offset, int64_t data_size_bytes, const T* data)
        -> void {
        const auto new_size_bytes = offset + data_size_bytes;

        if (this->capacity_bytes < new_size_bytes) {
            uint32_t new_shader_storage_buffer_id = 0;
            glCreateBuffers(1, &new_shader_storage_buffer_id);
            glNamedBufferData(
                new_shader_storage_buffer_id,
                new_size_bytes,
                nullptr,
                GL_DYNAMIC_DRAW
            );
            glBindBufferBase(
                GL_SHADER_STORAGE_BUFFER,
                static_cast<uint32_t>(this->binding_point),
                new_shader_storage_buffer_id
            );

            glCopyNamedBufferSubData(
                this->shader_storage_buffer_id,
                new_shader_storage_buffer_id,
                0,
                0,
                this->capacity_bytes
            );

            glDeleteBuffers(1, &this->shader_storage_buffer_id);

            this->shader_storage_buffer_id = new_shader_storage_buffer_id;
            this->capacity_bytes = new_size_bytes;
        }

        glNamedBufferSubData(
            this->shader_storage_buffer_id, offset, data_size_bytes, data
        );
    }

    OpenGLShaderStorageBuffer(const OpenGLShaderStorageBuffer&) = delete;
    OpenGLShaderStorageBuffer(OpenGLShaderStorageBuffer&&) = default;
    auto operator=(const OpenGLShaderStorageBuffer&)
        -> OpenGLShaderStorageBuffer& = delete;
    auto operator=(OpenGLShaderStorageBuffer&&)
        -> OpenGLShaderStorageBuffer& = default;

private:
    ShaderStorageBufferBindingPoint binding_point;
    uint32_t shader_storage_buffer_id = 0;
    int64_t capacity_bytes = 0;
};

}  // namespace Luminol::Graphics
