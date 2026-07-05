#pragma once

#include <memory>

#include <LuminolRenderEngine/Graphics/GraphicsFactory.hpp>

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

    [[nodiscard]] auto create_renderer(Window& window, GraphicsApi graphics_api)
        -> std::unique_ptr<Renderer> override;

    [[nodiscard]] auto create_mesh(
        gsl::span<const float> vertices,
        gsl::span<const uint32_t> indices,
        const TexturePaths& texture_paths
    ) const -> std::unique_ptr<Mesh> override;

    [[nodiscard]] auto create_model(const std::filesystem::path& model_path
    ) const -> std::unique_ptr<Model> override;

    [[nodiscard]] auto get_graphics_api() const -> GraphicsApi override;

private:
    std::unique_ptr<GPUDevice> gpu_device;
};

}  // namespace Luminol::Graphics::SDL_GPU
