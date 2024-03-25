#pragma once

#include <cstdint>
#include <string>

#include <glm/glm.hpp>

namespace Luminol {

class Window {
public:
    using WindowHandle = void*;
    using WindowProc = void (*(*)(const char*))();

    struct WindowHints {
        int32_t context_version_major = 0;
        int32_t context_version_minor = 0;
    };

    Window(
        int32_t width,
        int32_t height,
        const std::string& title,
        const WindowHints& hints
    );

    Window(const Window&) = delete;
    Window(Window&&) = default;
    auto operator=(const Window&) -> Window& = delete;
    auto operator=(Window&&) -> Window& = default;
    ~Window();

    [[nodiscard]] static auto get_proc_address() -> WindowProc;
    static auto poll_events() -> void;

    [[nodiscard]] auto should_close() const -> bool;
    auto swap_buffers() const -> void;

private:
    WindowHandle window_handle = nullptr;
};

}  // namespace Luminol
