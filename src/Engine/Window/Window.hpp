#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <functional>

namespace Luminol {

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

    auto poll_events() const -> void;

    [[nodiscard]] auto should_close() const -> bool;
    auto swap_buffers() const -> void;

    std::optional<FramebufferSizeCallback> framebuffer_size_callback =
        std::nullopt;

private:
    WindowHandle window_handle = nullptr;
};

}  // namespace Luminol
