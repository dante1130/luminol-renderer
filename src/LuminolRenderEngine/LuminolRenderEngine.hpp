#pragma once

#include <cstdint>
#include <string>
#include <memory>

#include <LuminolRenderEngine/Window/Window.hpp>
#include <LuminolRenderEngine/Graphics/Camera.hpp>
#include <LuminolRenderEngine/Graphics/Renderer.hpp>
#include <LuminolRenderEngine/Graphics/GraphicsFactory.hpp>

namespace Luminol {

constexpr static auto default_width = 640;
constexpr static auto default_height = 480;

struct Properties {
    int32_t width = {default_width};
    int32_t height = {default_height};
    std::string title = {"Luminol Engine"};
    Graphics::GraphicsApi graphics_api = {Graphics::GraphicsApi::OpenGL};
};

class RenderEngine {
public:
    RenderEngine(const Properties& properties);

    [[nodiscard]] auto get_window() const -> const Window&;
    [[nodiscard]] auto get_window() -> Window&;

    [[nodiscard]] auto get_camera() const -> const Graphics::Camera&;
    [[nodiscard]] auto get_camera() -> Graphics::Camera&;

    [[nodiscard]] auto get_renderer() const -> const Graphics::Renderer&;
    [[nodiscard]] auto get_renderer() -> Graphics::Renderer&;

    [[nodiscard]] auto get_graphics_factory() const
        -> const Graphics::GraphicsFactory&;

private:
    Window window;
    Graphics::Camera camera;
    std::unique_ptr<Graphics::GraphicsFactory> graphics_factory = nullptr;
    std::unique_ptr<Graphics::Renderer> renderer = nullptr;
};

}  // namespace Luminol
