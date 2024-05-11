#pragma once

#include <gsl/gsl>

#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLDrawCall.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLShader.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLShaderStorageBuffer.hpp>

namespace Luminol::Graphics {

class OpenGLColorRenderPass {
public:
    OpenGLColorRenderPass();

    auto draw(
        gsl::span<ColorDrawInstancedCall> draw_calls,
        OpenGLShaderStorageBuffer& instancing_model_matrix_buffer,
        OpenGLShaderStorageBuffer& instancing_color_buffer
    ) -> void;

private:
    OpenGLShader color_shader;
};

}  // namespace Luminol::Graphics
