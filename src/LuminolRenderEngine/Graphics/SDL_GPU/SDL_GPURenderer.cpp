#include "SDL_GPURenderer.hpp"

namespace Luminol::Graphics::SDL_GPU {

SDL_GPURenderer::SDL_GPURenderer(Window& window, GraphicsApi graphics_api)
    : Renderer(graphics_api),
      sdl_window{static_cast<SDL_Window*>(window.get_window_handle())},
      get_window_width([&window]() { return window.get_width(); }),
      get_window_height([&window]() { return window.get_height(); }),
      gpu_device{sdl_window} {}

}  // namespace Luminol::Graphics::SDL_GPU
