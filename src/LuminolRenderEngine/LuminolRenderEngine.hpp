#pragma once

#include <cstdint>
#include <string>
#include <memory>

#include <LuminolRenderEngine/Window/Window.hpp>
#include <LuminolRenderEngine/Graphics/Camera.hpp>
#include <LuminolRenderEngine/Graphics/RenderableManager.hpp>

namespace Luminol::Graphics::SDL_GPU {
class SDL_GPURenderer;
}  // namespace Luminol::Graphics::SDL_GPU

namespace Luminol {

constexpr static auto default_width = 640;
constexpr static auto default_height = 480;

struct Properties {
    int32_t width = {default_width};
    int32_t height = {default_height};
    std::string title = {"Luminol Engine"};
    // MSAA sample count; rounded down to the nearest supported power of
    // two, clamped further by device capability.
    uint32_t msaa_sample_count = 4;
};

class RenderEngine {
public:
    RenderEngine(const Properties& properties);
    // Declared out-of-line since SDL_GPURenderer is only forward-declared
    // here - std::unique_ptr's destructor needs the complete type, which is
    // only visible in the .cpp.
    ~RenderEngine();

    RenderEngine(const RenderEngine&) = delete;
    RenderEngine(RenderEngine&&) = default;
    auto operator=(const RenderEngine&) -> RenderEngine& = delete;
    auto operator=(RenderEngine&&) -> RenderEngine& = default;

    [[nodiscard]] auto get_window() const -> const Window&;
    [[nodiscard]] auto get_window() -> Window&;

    [[nodiscard]] auto get_renderer() const -> const Graphics::SDL_GPU::SDL_GPURenderer&;
    [[nodiscard]] auto get_renderer() -> Graphics::SDL_GPU::SDL_GPURenderer&;

private:
    Window window;
    std::unique_ptr<Graphics::SDL_GPU::SDL_GPURenderer> renderer = nullptr;
};

}  // namespace Luminol
