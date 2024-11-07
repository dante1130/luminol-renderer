#include "Window.hpp"

#include <iostream>

#include <gsl/gsl>
#include <GLFW/glfw3.h>

namespace {

constexpr auto window_handle_to_glfw_window(Luminol::Window::WindowHandle handle
) -> GLFWwindow* {
    return static_cast<GLFWwindow*>(handle);
}

auto framebuffer_size_callback_function(
    GLFWwindow* window, int32_t width, int32_t height
) -> void {
    auto* user_data = glfwGetWindowUserPointer(window);
    auto* luminol_window = reinterpret_cast<Luminol::Window*>(user_data);

    const auto& framebuffer_size_callback =
        luminol_window->get_framebuffer_size_callback();

    if (framebuffer_size_callback.has_value()) {
        framebuffer_size_callback.value()(width, height);
    }
}

constexpr auto key_event_to_glfw_key_event(Luminol::KeyEvent event) -> int32_t {
    switch (event) {
        case Luminol::KeyEvent::Press:
            return GLFW_PRESS;
        case Luminol::KeyEvent::Release:
            return GLFW_RELEASE;
        case Luminol::KeyEvent::Repeat:
            return GLFW_REPEAT;
        default:
            return GLFW_RELEASE;
    }
}

}  // namespace

namespace Luminol {

Window::Window(int32_t width, int32_t height, const std::string& title) {
    if (glfwInit() == GLFW_FALSE) {
        std::cerr << "Failed to initialize GLFW\n";
        Ensures(false);
    }

    this->window_handle =
        glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);

    if (this->window_handle == nullptr) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        Ensures(false);
    }

    glfwMakeContextCurrent(window_handle_to_glfw_window(window_handle));
    glfwSetWindowUserPointer(window_handle_to_glfw_window(window_handle), this);
    glfwSetFramebufferSizeCallback(
        window_handle_to_glfw_window(window_handle),
        framebuffer_size_callback_function
    );

    glfwSetInputMode(
        window_handle_to_glfw_window(window_handle),
        GLFW_CURSOR,
        GLFW_CURSOR_DISABLED
    );

    if (glfwRawMouseMotionSupported() == GLFW_TRUE) {
        glfwSetInputMode(
            window_handle_to_glfw_window(window_handle),
            GLFW_RAW_MOUSE_MOTION,
            GLFW_TRUE
        );
    }
}

Window::~Window() { glfwTerminate(); }

auto Window::get_width() const -> int32_t {
    int32_t width = 0;
    int32_t height = 0;
    glfwGetWindowSize(
        window_handle_to_glfw_window(window_handle), &width, &height
    );

    return width;
}

auto Window::get_height() const -> int32_t {
    int32_t width = 0;
    int32_t height = 0;
    glfwGetWindowSize(
        window_handle_to_glfw_window(window_handle), &width, &height
    );

    return height;
}

// NOLINTBEGIN(readability-convert-member-functions-to-static)
auto Window::get_proc_address() const -> Window::WindowProc {
    return glfwGetProcAddress;
}
// NOLINTEND(readability-convert-member-functions-to-static)

auto Window::is_key_event(int32_t key, KeyEvent event) const -> bool {
    return glfwGetKey(window_handle_to_glfw_window(window_handle), key) ==
           key_event_to_glfw_key_event(event);
}

auto Window::get_mouse_delta() -> MouseDelta {
    double mouse_x = 0.0;
    double mouse_y = 0.0;
    glfwGetCursorPos(
        window_handle_to_glfw_window(window_handle), &mouse_x, &mouse_y
    );

    if (first_mouse) {
        last_mouse_x = mouse_x;
        last_mouse_y = mouse_y;
        first_mouse = false;

        return {.delta_x = 0.0, .delta_y = 0.0};
    }

    const auto delta_x = mouse_x - last_mouse_x;
    const auto delta_y = last_mouse_y - mouse_y;

    last_mouse_x = mouse_x;
    last_mouse_y = mouse_y;

    return {.delta_x = delta_x, .delta_y = delta_y};
}

// NOLINTBEGIN(readability-convert-member-functions-to-static)
auto Window::poll_events() const -> void { glfwPollEvents(); }
// NOLINTEND(readability-convert-member-functions-to-static)

auto Window::get_framebuffer_size_callback() const
    -> const std::optional<FramebufferSizeCallback>& {
    return this->framebuffer_size_callback;
}

auto Window::set_framebuffer_size_callback(
    const FramebufferSizeCallback& callback
) -> void {
    this->framebuffer_size_callback = callback;
}

auto Window::should_close() const -> bool {
    return glfwWindowShouldClose(window_handle_to_glfw_window(window_handle)) ==
           GLFW_TRUE;
}

auto Window::close() const -> void {
    glfwSetWindowShouldClose(
        window_handle_to_glfw_window(window_handle), GLFW_TRUE
    );
}

auto Window::swap_buffers() const -> void {
    glfwSwapBuffers(window_handle_to_glfw_window(window_handle));
}

}  // namespace Luminol
