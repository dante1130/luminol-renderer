#pragma once

#include <unordered_map>
#include <vector>

#include <LuminolRenderEngine/Graphics/GraphicsFactory.hpp>
#include <LuminolRenderEngine/Graphics/RenderableManager.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLMesh.hpp>

namespace Luminol::Graphics {

class OpenGLFactory : public GraphicsFactory {
public:
    [[nodiscard]] auto create_renderer(Window& window)
        -> std::unique_ptr<Renderer> override;

    [[nodiscard]] auto create_mesh(
        gsl::span<const float> vertices,
        gsl::span<const uint32_t> indices,
        const TexturePaths& texture_paths
    ) -> RenderableId override;

    [[nodiscard]] auto create_model(const std::filesystem::path& model_path)
        -> RenderableId override;

    auto remove_renderable(RenderableId renderable_id) -> void override;

    [[nodiscard]] auto get_graphics_api() const -> GraphicsApi override;

    [[nodiscard]] auto get_meshes(RenderableId renderable_id) const
        -> gsl::span<const OpenGLMesh>;

private:
    RenderableManager renderable_manager;
    std::unordered_map<RenderableId, std::vector<OpenGLMesh>> meshes_by_id;
};

}  // namespace Luminol::Graphics
