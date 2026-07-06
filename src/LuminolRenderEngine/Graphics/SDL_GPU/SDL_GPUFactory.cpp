#include "SDL_GPUFactory.hpp"

#include <gsl/gsl>
#include <SDL3/SDL_video.h>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUDevice.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUMesh.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUModel.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPURenderer.hpp>

namespace Luminol::Graphics::SDL_GPU {

SDL_GPUFactory::SDL_GPUFactory() = default;
SDL_GPUFactory::~SDL_GPUFactory() = default;

auto SDL_GPUFactory::create_renderer(Window& window)
    -> std::unique_ptr<Renderer> {
    auto* sdl_window = static_cast<SDL_Window*>(window.get_window_handle());
    gpu_device = std::make_shared<GPUDevice>(sdl_window);
    return std::make_unique<SDL_GPURenderer>(
        window, shared_from_this(), gpu_device
    );
}

auto SDL_GPUFactory::get_gpu_device() const -> std::shared_ptr<GPUDevice> {
    return gpu_device;
}

auto SDL_GPUFactory::create_mesh(
    gsl::span<const float> vertices,
    gsl::span<const uint32_t> indices,
    const TexturePaths& texture_paths
) const -> std::unique_ptr<Mesh> {
    Expects(gpu_device != nullptr);
    return std::make_unique<SDL_GPUMesh>(
        *gpu_device, vertices, indices, texture_paths
    );
}

auto SDL_GPUFactory::create_model(const std::filesystem::path& model_path
) const -> std::unique_ptr<Model> {
    Expects(gpu_device != nullptr);
    return std::make_unique<SDL_GPUModel>(*gpu_device, model_path);
}

auto SDL_GPUFactory::get_graphics_api() const -> GraphicsApi {
    return GraphicsApi::SDL_GPU;
}

}  // namespace Luminol::Graphics::SDL_GPU
