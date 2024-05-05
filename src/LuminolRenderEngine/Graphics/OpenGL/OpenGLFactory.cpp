#include "OpenGLFactory.hpp"

#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLRenderer.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLShader.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLMesh.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLModel.hpp>

namespace Luminol::Graphics {

auto OpenGLFactory::create_renderer(Window& window) const
    -> std::unique_ptr<Renderer> {
    return std::make_unique<OpenGLRenderer>(window);
}

auto OpenGLFactory::create_mesh(
    gsl::span<const float> vertices,
    gsl::span<const uint32_t> indices,
    const TexturePaths& texture_paths
) const -> std::unique_ptr<Mesh> {
    return std::make_unique<OpenGLMesh>(vertices, indices, texture_paths);
}

auto OpenGLFactory::create_model(const std::filesystem::path& model_path) const
    -> std::unique_ptr<Model> {
    return std::make_unique<OpenGLModel>(model_path);
}

auto OpenGLFactory::get_graphics_api() const -> GraphicsApi {
    return GraphicsApi::OpenGL;
}

}  // namespace Luminol::Graphics
