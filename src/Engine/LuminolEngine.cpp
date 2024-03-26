#include "LuminolEngine.hpp"

#include <Engine/Graphics/OpenGL/OpenGLRenderer.hpp>

namespace {

using namespace Luminol;

constexpr auto make_window_hints() -> Window::WindowHints {
    constexpr auto window_hints = Window::WindowHints{
        .context_version_major = 4,
        .context_version_minor = 6,
    };

    return window_hints;
}

}  // namespace

namespace Luminol {

Engine::Engine(const Properties& properties)
    : window(
          properties.width,
          properties.height,
          properties.title,
          make_window_hints()
      ) {
    this->renderer = std::make_unique<Graphics::OpenGLRenderer>(this->window);
}

void Engine::run() {
    while (!this->window.should_close()) {
        this->window.poll_events();

        constexpr auto color = glm::vec4(0.2f, 0.3f, 0.3f, 1.0f);

        this->renderer->clear_color(color);
        this->renderer->clear(Luminol::Graphics::BufferBit::Color);

        this->window.swap_buffers();
    }
}

}  // namespace Luminol
