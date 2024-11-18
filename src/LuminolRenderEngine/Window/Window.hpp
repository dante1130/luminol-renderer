#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <functional>

namespace Luminol {

enum class KeyEvent : uint8_t { Press = 0, Release };

struct MouseDelta {
    double delta_x = {0.0};
    double delta_y = {0.0};
};

class Window {
public:
    using WindowHandle = void*;
    using InitStateHandle = void*;
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

    [[nodiscard]] auto is_key_event(uint32_t key, KeyEvent event) const -> bool;
    [[nodiscard]] auto get_mouse_delta() const -> MouseDelta;

    auto poll_events() -> void;

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
    std::unique_ptr<InitStateHandle> init_state_handle = nullptr;

    std::unordered_map<uint32_t, KeyEvent> key_states;

    MouseDelta mouse_delta = {.delta_x = 0.0, .delta_y = 0.0};
};

}  // namespace Luminol
