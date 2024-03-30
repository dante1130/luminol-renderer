#include "Window.hpp"

#include <cassert>

#include <GLFW/glfw3.h>

namespace {

constexpr auto window_handle_to_glfw_window(Luminol::Window::WindowHandle handle
) -> GLFWwindow* {
    return static_cast<GLFWwindow*>(handle);
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
}

Window::~Window() { glfwTerminate(); }

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
