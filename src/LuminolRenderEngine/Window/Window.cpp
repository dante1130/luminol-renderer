#include "Window.hpp"

#include <gsl/gsl>
#include <SDL3/SDL.h>

namespace {

constexpr auto window_handle_to_sdl_window(Luminol::Window::WindowHandle handle)
    -> SDL_Window* {
    return static_cast<SDL_Window*>(handle);
}

/* auto framebuffer_size_callback_function(
    GLFWwindow* window, int32_t width, int32_t height
) -> void {
    auto* user_data = glfwGetWindowUserPointer(window);
    auto* luminol_window = reinterpret_cast<Luminol::Window*>(user_data);

    const auto& framebuffer_size_callback =
        luminol_window->get_framebuffer_size_callback();

    if (framebuffer_size_callback.has_value()) {
        framebuffer_size_callback.value()(width, height);
    }
} */

}  // namespace

namespace Luminol {

Window::Window(int32_t width, int32_t height, const std::string& title) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_LogError(
            SDL_LOG_CATEGORY_ERROR,
            "Failed to initialize SDL: %s",
            SDL_GetError()
        );
        Ensures(false);
    }

    this->window_handle = SDL_CreateWindow(
        title.c_str(), width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );

    if (this->window_handle == nullptr) {
        SDL_LogError(
            SDL_LOG_CATEGORY_ERROR,
            "Failed to create SDL window: %s",
            SDL_GetError()
        );
        Ensures(false);
    }

    SDL_GL_CreateContext(window_handle_to_sdl_window(this->window_handle));

    SDL_SetInitialized(this->sdl_state.get(), true);
}

Window::~Window() { SDL_Quit(); }

auto Window::get_width() const -> int32_t {
    int32_t width = 0;
    int32_t height = 0;
    SDL_GetWindowSize(
        window_handle_to_sdl_window(window_handle), &width, &height
    );

    return width;
}

auto Window::get_height() const -> int32_t {
    int32_t width = 0;
    int32_t height = 0;
    SDL_GetWindowSize(
        window_handle_to_sdl_window(window_handle), &width, &height
    );

    return height;
}

// NOLINTBEGIN(readability-convert-member-functions-to-static)
auto Window::get_proc_address() const -> Window::WindowProc {
    return SDL_GL_GetProcAddress;
}
// NOLINTEND(readability-convert-member-functions-to-static)

auto Window::is_key_event(uint32_t key, KeyEvent event) const -> bool {
    return this->key_states.contains(key) && this->key_states.at(key) == event;
}

auto Window::get_mouse_delta() const -> MouseDelta { return this->mouse_delta; }

auto Window::poll_events() -> void {
    this->key_states.clear();

    if (!SDL_HasEvent(SDL_EVENT_MOUSE_MOTION)) {
        this->mouse_delta.delta_x = 0.0;
        this->mouse_delta.delta_y = 0.0;
    }

    auto sdl_event = SDL_Event{};

    while (SDL_PollEvent(&sdl_event)) {
        switch (sdl_event.type) {
            case SDL_EVENT_QUIT:
                this->close();
                break;

            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP: {
                if (sdl_event.key.key < SDLK_A && sdl_event.key.key > SDLK_Z) {
                    break;
                }

                const auto event = sdl_event.key.type == SDL_EVENT_KEY_DOWN
                                       ? KeyEvent::Press
                                       : KeyEvent::Release;

                this->key_states[sdl_event.key.key] = event;
                break;
            }

            case SDL_EVENT_MOUSE_MOTION: {
                this->mouse_delta.delta_x = sdl_event.motion.xrel;
                this->mouse_delta.delta_y = -sdl_event.motion.yrel;
                break;
            }

            default:
                break;
        }
    }
}

auto Window::get_framebuffer_size_callback() const
    -> const std::optional<FramebufferSizeCallback>& {
    return this->framebuffer_size_callback;
}

auto Window::set_framebuffer_size_callback(
    const FramebufferSizeCallback& callback
) -> void {
    this->framebuffer_size_callback = callback;
}

auto Window::should_close() -> bool {
    const auto status = SDL_GetAtomicInt(&this->sdl_state->status);

    return status == SDL_INIT_STATUS_UNINITIALIZING ||
           status == SDL_INIT_STATUS_UNINITIALIZED;
}

auto Window::close() -> void {
    SDL_DestroyWindow(window_handle_to_sdl_window(window_handle));
    SDL_SetInitialized(this->sdl_state.get(), false);
}

auto Window::swap_buffers() const -> void {
    SDL_GL_SwapWindow(window_handle_to_sdl_window(window_handle));
}

}  // namespace Luminol
