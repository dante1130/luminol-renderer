#include "OpenGLFactory.hpp"

#include <Engine/Graphics/OpenGL/OpenGLRenderer.hpp>
#include <Engine/Graphics/OpenGL/OpenGLShader.hpp>
#include <Engine/Graphics/OpenGL/OpenGLMesh.hpp>

namespace Luminol::Graphics {

auto OpenGLFactory::create_renderer(const Window& window)
    -> std::unique_ptr<Renderer> {
    return std::make_unique<OpenGLRenderer>(window);
}

auto OpenGLFactory::create_shader(const ShaderPaths& paths)
    -> std::unique_ptr<Shader> {
    return std::make_unique<OpenGLShader>(paths);
}

auto OpenGLFactory::create_mesh(
    gsl::span<const float> vertices,
    gsl::span<const uint32_t> indices,
    const std::filesystem::path& texture_path
) -> std::unique_ptr<Mesh> {
    return std::make_unique<OpenGLMesh>(vertices, indices, texture_path);
}

auto OpenGLFactory::get_graphics_api() const -> GraphicsApi {
    return GraphicsApi::OpenGL;
}

}  // namespace Luminol::Graphics
