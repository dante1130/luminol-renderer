#include "OpenGLFactory.hpp"

#include <Engine/Graphics/OpenGL/OpenGLRenderer.hpp>
#include <Engine/Graphics/OpenGL/OpenGLShader.hpp>
#include <Engine/Graphics/OpenGL/OpenGLMesh.hpp>
#include <Engine/Graphics/OpenGL/OpenGLModel.hpp>

namespace Luminol::Graphics {

auto OpenGLFactory::create_renderer(Window& window)
    -> std::unique_ptr<Renderer> {
    return std::make_unique<OpenGLRenderer>(window);
}

auto OpenGLFactory::create_mesh(
    gsl::span<const float> vertices,
    gsl::span<const uint32_t> indices,
    const TexturePaths& texture_paths
) -> std::unique_ptr<Mesh> {
    return std::make_unique<OpenGLMesh>(vertices, indices, texture_paths);
}

auto OpenGLFactory::create_model(const std::filesystem::path& model_path)
    -> std::unique_ptr<Model> {
    return std::make_unique<OpenGLModel>(model_path);
}

auto OpenGLFactory::get_graphics_api() const -> GraphicsApi {
    return GraphicsApi::OpenGL;
}

}  // namespace Luminol::Graphics
