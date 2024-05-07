#pragma once

#include <cstdint>

#include <glad/gl.h>
#include <gsl/gsl>

#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLUniformBindingPoints.hpp>

namespace Luminol::Graphics {

template <typename T>
class OpenGLShaderStorageBuffer {
public:
    OpenGLShaderStorageBuffer(
        gsl::span<T> data, ShaderStorageBufferBindingPoint binding_point
    ) {
        glCreateBuffers(1, &this->shader_storage_buffer_id);
        glNamedBufferData(
            this->shader_storage_buffer_id,
            data.size_bytes(),
            data.data(),
            GL_DYNAMIC_DRAW
        );
        glBindBufferBase(
            GL_SHADER_STORAGE_BUFFER,
            static_cast<uint32_t>(binding_point),
            this->shader_storage_buffer_id
        );
    }

    OpenGLShaderStorageBuffer(
        const T& data, ShaderStorageBufferBindingPoint binding_point
    ) {
        glCreateBuffers(1, &this->shader_storage_buffer_id);
        glNamedBufferData(
            this->shader_storage_buffer_id, sizeof(T), &data, GL_DYNAMIC_DRAW
        );
        glBindBufferBase(
            GL_SHADER_STORAGE_BUFFER,
            static_cast<uint32_t>(binding_point),
            this->shader_storage_buffer_id
        );
    }

    ~OpenGLShaderStorageBuffer() {
        glDeleteBuffers(1, &this->shader_storage_buffer_id);
    }

    auto set_data(gsl::span<T> data) -> void {
        glNamedBufferSubData(
            this->shader_storage_buffer_id, 0, data.size_bytes(), data.data()
        );
    }

    auto set_data(const T& data) -> void {
        glNamedBufferSubData(
            this->shader_storage_buffer_id, 0, sizeof(T), &data
        );
    }

    OpenGLShaderStorageBuffer(const OpenGLShaderStorageBuffer&) = delete;
    OpenGLShaderStorageBuffer(OpenGLShaderStorageBuffer&&) = default;
    auto operator=(const OpenGLShaderStorageBuffer&)
        -> OpenGLShaderStorageBuffer& = delete;
    auto operator=(OpenGLShaderStorageBuffer&&)
        -> OpenGLShaderStorageBuffer& = default;

private:
    uint32_t shader_storage_buffer_id = 0;
};

}  // namespace Luminol::Graphics
