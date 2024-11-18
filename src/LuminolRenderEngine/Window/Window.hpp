#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <functional>

struct SDL_InitState;

namespace Luminol {

enum class KeyEvent : uint8_t { Press = 0, Release = 1, Repeat = 2 };

struct MouseDelta {
    double delta_x = {0.0};
    double delta_y = {0.0};
};

class Window {
public:
    using WindowHandle = void*;
    using WindowProc = void (*(*)(const char*))();
    using FramebufferSizeCallback = std::function<void(int32_t, int32_t)>;

    Window(int32_t width, int32_t height, const std::string& title);

    Window(const Window&) = delete;
    Window(Window&&) = default;
    auto operator=(const Window&) -> Window& = delete;
    auto operator=(Window&&) -> Window& = default;
    ~Window();

    [[nodiscard]] auto get_width() const -> int32_t;
    [[nodiscard]] auto get_height() const -> int32_t;

    [[nodiscard]] auto get_proc_address() const -> WindowProc;

    [[nodiscard]] auto is_key_event(int32_t key, KeyEvent event) const -> bool;
    [[nodiscard]] auto get_mouse_delta() -> MouseDelta;

    auto poll_events() const -> void;

    [[nodiscard]] auto get_framebuffer_size_callback() const
        -> const std::optional<FramebufferSizeCallback>&;
    auto set_framebuffer_size_callback(const FramebufferSizeCallback& callback)
        -> void;

    [[nodiscard]] auto should_close() -> bool;
    auto close() -> void;

    auto swap_buffers() const -> void;

private:
    std::optional<FramebufferSizeCallback> framebuffer_size_callback =
        std::nullopt;
    WindowHandle window_handle = nullptr;
    std::unique_ptr<SDL_InitState> sdl_state =
        std::make_unique<SDL_InitState>();

    double last_mouse_x = {0.0};
    double last_mouse_y = {0.0};

    bool first_mouse = {true};
};

}  // namespace Luminol
