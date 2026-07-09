#pragma once

#include <LuminolMaths/Matrix.hpp>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUGraphicsPipeline.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUShader.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUSkybox.hpp>

namespace Luminol::Graphics::SDL_GPU {

class GPUDevice;
class CommandBuffer;
class RenderPass;

// Draws the default skybox as a fullscreen triangle into the main forward
// pass, after opaque geometry. The vertex shader reconstructs a world-space
// view ray per pixel from the inverse of the translation-stripped
// view-projection matrix and forces depth to the far plane, so it only
// shows through where the depth buffer wasn't already written by opaque
// geometry (mirrors OpenGLSkyboxRenderPass's gl_Position.xyww / GL_LEQUAL
// trick, without needing a cube mesh or a front-face culling flip).
class SDL_GPUSkyboxRenderPass {
public:
    SDL_GPUSkyboxRenderPass(GPUDevice& device, SampleCount sample_count);

    auto draw(
        CommandBuffer& command_buffer,
        RenderPass& render_pass,
        const Maths::Matrix4x4f& view_matrix,
        const Maths::Matrix4x4f& projection_matrix
    ) const -> void;

private:
    Shader skybox_vertex_shader;
    Shader skybox_fragment_shader;
    GraphicsPipeline skybox_pipeline;

    SDL_GPUSkybox skybox;
};

}  // namespace Luminol::Graphics::SDL_GPU
