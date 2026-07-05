#include "SDL_GPUFactory.hpp"

#include <stdexcept>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPURenderer.hpp>

namespace Luminol::Graphics::SDL_GPU {

auto SDL_GPUFactory::create_renderer(
    Window& window, GraphicsApi graphics_api
) const -> std::unique_ptr<Renderer> {
    return std::make_unique<SDL_GPURenderer>(window, graphics_api);
}

auto SDL_GPUFactory::create_mesh(
    gsl::span<const float> /*vertices*/,
    gsl::span<const uint32_t> /*indices*/,
    const TexturePaths& /*texture_paths*/
) const -> std::unique_ptr<Mesh> {
    throw std::runtime_error("SDL_GPU mesh creation not implemented yet");
}

auto SDL_GPUFactory::create_model(
    const std::filesystem::path& /*model_path*/
) const -> std::unique_ptr<Model> {
    throw std::runtime_error("SDL_GPU model creation not implemented yet");
}

auto SDL_GPUFactory::get_graphics_api() const -> GraphicsApi {
    return GraphicsApi::SDL_GPU;
}

}  // namespace Luminol::Graphics::SDL_GPU
