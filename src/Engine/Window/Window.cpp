#include "Window.hpp"

#include <cassert>
#include <bit>

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
    auto* luminol_window = std::bit_cast<Luminol::Window*>(user_data);

    if (luminol_window->framebuffer_size_callback.has_value()) {
        luminol_window->framebuffer_size_callback.value()(width, height);
    }
}

}  // namespace

namespace Luminol {

Window::Window(int32_t width, int32_t height, const std::string& title) {
    assert(glfwInit() == GLFW_TRUE && "Failed to initialize GLFW");

    this->window_handle =
        glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);

    if (this->window_handle == nullptr) {
        glfwTerminate();
    }

    glfwMakeContextCurrent(window_handle_to_glfw_window(window_handle));
    glfwSetWindowUserPointer(window_handle_to_glfw_window(window_handle), this);
    glfwSetFramebufferSizeCallback(
        window_handle_to_glfw_window(window_handle),
        framebuffer_size_callback_function
    );
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

auto Window::poll_events() const -> void { glfwPollEvents(); }
// NOLINTEND(readability-convert-member-functions-to-static)

auto Window::should_close() const -> bool {
    return glfwWindowShouldClose(window_handle_to_glfw_window(window_handle)) ==
           GLFW_TRUE;
}

auto Window::swap_buffers() const -> void {
    glfwSwapBuffers(window_handle_to_glfw_window(window_handle));
}

}  // namespace Luminol
