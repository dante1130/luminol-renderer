#pragma once

#include <gsl/gsl>

#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLDrawCall.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLShader.hpp>

namespace Luminol::Graphics {

class OpenGLRenderer;

class OpenGLColorRenderPass {
public:
    OpenGLColorRenderPass();

    auto draw(OpenGLRenderer& renderer, gsl::span<ColorDrawCall> draw_calls)
        -> void;

private:
    OpenGLShader color_shader;
};

}  // namespace Luminol::Graphics
