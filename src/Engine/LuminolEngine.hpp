#pragma once

#include <cstdint>
#include <string>
#include <memory>

#include <Engine/Window/Window.hpp>
#include <Engine/Graphics/Camera.hpp>
#include <Engine/Graphics/Renderer.hpp>
#include <Engine/Graphics/GraphicsFactory.hpp>
#include <Engine/Utilities/Timer.hpp>

namespace Luminol {

constexpr static auto default_width = 640;
constexpr static auto default_height = 480;

struct Properties {
    int32_t width = {default_width};
    int32_t height = {default_height};
    std::string title = {"Luminol Engine"};
    Graphics::GraphicsApi graphics_api = {Graphics::GraphicsApi::OpenGL};
};

class Engine {
public:
    Engine(const Properties& properties);

    [[nodiscard]] auto get_window() const -> const Window&;
    [[nodiscard]] auto get_window() -> Window&;

    [[nodiscard]] auto get_camera() const -> const Graphics::Camera&;
    [[nodiscard]] auto get_camera() -> Graphics::Camera&;

    [[nodiscard]] auto get_renderer() const -> const Graphics::Renderer&;
    [[nodiscard]] auto get_renderer() -> Graphics::Renderer&;

    [[nodiscard]] auto get_graphics_factory() const
        -> const Graphics::GraphicsFactory&;

    [[nodiscard]] auto get_delta_time_seconds() const -> double;

    auto handle_key_events() -> void;

private:
    Window window;
    Graphics::Camera camera;
    std::unique_ptr<Graphics::GraphicsFactory> graphics_factory = nullptr;
    std::unique_ptr<Graphics::Renderer> renderer = nullptr;
    Utilities::Timer timer;

    double delta_time_seconds = {0.0};
    double last_frame_time_seconds = {0.0};
};

}  // namespace Luminol
