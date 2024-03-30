#pragma once

#include <cstdint>
#include <string>

namespace Luminol {

class Window {
public:
    using WindowHandle = void*;
    using WindowProc = void (*(*)(const char*))();

    Window(int32_t width, int32_t height, const std::string& title);

    Window(const Window&) = delete;
    Window(Window&&) = default;
    auto operator=(const Window&) -> Window& = delete;
    auto operator=(Window&&) -> Window& = default;
    ~Window();

    [[nodiscard]] auto get_proc_address() const -> WindowProc;
    auto poll_events() const -> void;

    [[nodiscard]] auto should_close() const -> bool;
    auto swap_buffers() const -> void;

private:
    WindowHandle window_handle = nullptr;
};

}  // namespace Luminol
