#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include <LuminolRenderEngine/Graphics/GraphicsFactory.hpp>
#include <LuminolRenderEngine/Graphics/RenderableManager.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUFont.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUMesh.hpp>

namespace Luminol::Graphics::SDL_GPU {

class GPUDevice;

class SDL_GPUFactory : public GraphicsFactory {
public:
    SDL_GPUFactory();
    ~SDL_GPUFactory() override;

    SDL_GPUFactory(const SDL_GPUFactory&) = delete;
    SDL_GPUFactory(SDL_GPUFactory&&) = delete;
    auto operator=(const SDL_GPUFactory&) -> SDL_GPUFactory& = delete;
    auto operator=(SDL_GPUFactory&&) -> SDL_GPUFactory& = delete;

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

    [[nodiscard]] auto get_gpu_device() const -> std::shared_ptr<GPUDevice>;

    [[nodiscard]] auto get_meshes(RenderableId renderable_id) const
        -> gsl::span<const SDL_GPUMesh>;

    [[nodiscard]] auto create_font(
        const std::filesystem::path& font_path, float point_size
    ) -> FontId override;

    [[nodiscard]] auto get_font(FontId font_id) const -> const SDL_GPUFont&;

private:
    std::shared_ptr<GPUDevice> gpu_device;
    RenderableManager renderable_manager;
    std::unordered_map<RenderableId, std::vector<SDL_GPUMesh>> meshes_by_id;

    RenderableManager font_manager;
    std::unordered_map<FontId, SDL_GPUFont> fonts_by_id;
};

}  // namespace Luminol::Graphics::SDL_GPU
