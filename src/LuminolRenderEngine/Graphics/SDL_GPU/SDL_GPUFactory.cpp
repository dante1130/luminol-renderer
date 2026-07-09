#include "SDL_GPUFactory.hpp"

#include <gsl/gsl>
#include <SDL3/SDL_video.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCommandBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUDevice.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPURenderer.hpp>

namespace Luminol::Graphics::SDL_GPU {

SDL_GPUFactory::SDL_GPUFactory() { Expects(TTF_Init()); }

SDL_GPUFactory::~SDL_GPUFactory() {
    fonts_by_id.clear();
    TTF_Quit();
}

auto SDL_GPUFactory::create_renderer(Window& window)
    -> std::unique_ptr<Renderer> {
    auto* sdl_window = static_cast<SDL_Window*>(window.get_window_handle());
    gpu_device = std::make_shared<GPUDevice>(sdl_window);
    return std::make_unique<SDL_GPURenderer>(
        window,
        std::static_pointer_cast<SDL_GPUFactory>(shared_from_this()),
        gpu_device
    );
}

auto SDL_GPUFactory::get_gpu_device() const -> std::shared_ptr<GPUDevice> {
    return gpu_device;
}

auto SDL_GPUFactory::create_mesh(
    gsl::span<const float> vertices,
    gsl::span<const uint32_t> indices,
    const TexturePaths& texture_paths
) -> RenderableId {
    Expects(gpu_device != nullptr);

    const auto renderable_id = this->renderable_manager.allocate_id();

    auto meshes = std::vector<SDL_GPUMesh>{};

    auto command_buffer = gpu_device->create_command_buffer();
    {
        auto copy_pass = command_buffer.begin_copy_pass();
        meshes.emplace_back(
            *gpu_device, copy_pass, vertices, indices, texture_paths
        );
    }
    command_buffer.submit();

    this->meshes_by_id.emplace(renderable_id, std::move(meshes));

    return renderable_id;
}

auto SDL_GPUFactory::create_model(const std::filesystem::path& model_path)
    -> RenderableId {
    Expects(gpu_device != nullptr);

    const auto renderable_id = this->renderable_manager.allocate_id(model_path);

    if (this->meshes_by_id.contains(renderable_id)) {
        return renderable_id;
    }

    this->meshes_by_id.emplace(
        renderable_id, load_meshes_from_model(*gpu_device, model_path)
    );

    return renderable_id;
}

auto SDL_GPUFactory::remove_renderable(RenderableId renderable_id) -> void {
    this->meshes_by_id.erase(renderable_id);
    this->renderable_manager.remove_renderable(renderable_id);
}

auto SDL_GPUFactory::get_graphics_api() const -> GraphicsApi {
    return GraphicsApi::SDL_GPU;
}

auto SDL_GPUFactory::get_meshes(RenderableId renderable_id) const
    -> gsl::span<const SDL_GPUMesh> {
    return this->meshes_by_id.at(renderable_id);
}

auto SDL_GPUFactory::create_font(
    const std::filesystem::path& font_path, float point_size
) -> FontId {
    const auto font_id = this->font_manager.allocate_id(font_path);

    if (this->fonts_by_id.contains(font_id)) {
        return font_id;
    }

    this->fonts_by_id.emplace(font_id, SDL_GPUFont{font_path, point_size});

    return font_id;
}

auto SDL_GPUFactory::get_font(FontId font_id) const -> const SDL_GPUFont& {
    return this->fonts_by_id.at(font_id);
}

}  // namespace Luminol::Graphics::SDL_GPU
