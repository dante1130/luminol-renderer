#pragma once

#include <gsl/gsl>

#include <LuminolRenderEngine/Graphics/RenderableManager.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLDrawCall.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLFrameBuffer.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLShader.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLShaderStorageBuffer.hpp>

namespace Luminol::Graphics {

class OpenGLGBufferRenderPass {
public:
    OpenGLGBufferRenderPass(int32_t width, int32_t height);

    [[nodiscard]] auto get_gbuffer_frame_buffer() -> OpenGLFrameBuffer&;

    auto draw(
        const RenderableManager& renderable_manager,
        gsl::span<InstancedDrawCall> draw_calls,
        OpenGLShaderStorageBuffer& instancing_model_matrix_buffer
    ) const -> void;

private:
    OpenGLFrameBuffer gbuffer_frame_buffer;
    OpenGLShader gbuffer_shader;
};

}  // namespace Luminol::Graphics
