#include "OpenGLFactory.hpp"

#include <Engine/Graphics/OpenGL/OpenGLRenderer.hpp>
#include <Engine/Graphics/OpenGL/OpenGLShader.hpp>

namespace Luminol::Graphics {

auto OpenGLFactory::create_renderer(const Window& window)
    -> std::unique_ptr<Renderer> {
    return std::make_unique<OpenGLRenderer>(window);
}

auto OpenGLFactory::create_shader(const ShaderPaths& paths)
    -> std::unique_ptr<Shader> {
    return std::make_unique<OpenGLShader>(paths);
}

auto OpenGLFactory::get_graphics_api() const -> GraphicsApi {
    return GraphicsApi::OpenGL;
}

}  // namespace Luminol::Graphics
