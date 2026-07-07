#include "OpenGLFactory.hpp"

#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLRenderer.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLShader.hpp>

namespace Luminol::Graphics {

auto OpenGLFactory::create_renderer(Window& window)
    -> std::unique_ptr<Renderer> {
    return std::make_unique<OpenGLRenderer>(
        window, std::static_pointer_cast<OpenGLFactory>(shared_from_this())
    );
}

auto OpenGLFactory::create_mesh(
    gsl::span<const float> vertices,
    gsl::span<const uint32_t> indices,
    const TexturePaths& texture_paths
) -> RenderableId {
    const auto renderable_id = this->renderable_manager.allocate_id();

    auto meshes = std::vector<OpenGLMesh>{};
    meshes.emplace_back(vertices, indices, texture_paths);
    this->meshes_by_id.emplace(renderable_id, std::move(meshes));

    return renderable_id;
}

auto OpenGLFactory::create_model(const std::filesystem::path& model_path)
    -> RenderableId {
    const auto renderable_id = this->renderable_manager.allocate_id(model_path);

    if (this->meshes_by_id.contains(renderable_id)) {
        return renderable_id;
    }

    this->meshes_by_id.emplace(
        renderable_id, load_meshes_from_model(model_path)
    );

    return renderable_id;
}

auto OpenGLFactory::remove_renderable(RenderableId renderable_id) -> void {
    this->meshes_by_id.erase(renderable_id);
    this->renderable_manager.remove_renderable(renderable_id);
}

auto OpenGLFactory::get_graphics_api() const -> GraphicsApi {
    return GraphicsApi::OpenGL;
}

auto OpenGLFactory::get_meshes(RenderableId renderable_id) const
    -> gsl::span<const OpenGLMesh> {
    return this->meshes_by_id.at(renderable_id);
}

}  // namespace Luminol::Graphics
