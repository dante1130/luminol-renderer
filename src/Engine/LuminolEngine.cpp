#include "LuminolEngine.hpp"

#include <Engine/Graphics/OpenGL/OpenGLFactory.hpp>

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
      ),
      graphics_factory(std::make_unique<Graphics::OpenGLFactory>()),
      renderer(this->graphics_factory->create_renderer(this->window)) {}

void Engine::run() {
    constexpr auto vertices =
        std::array{-0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.0f, 0.0f, 0.5f, 0.0f};

    const auto mesh = this->graphics_factory->create_mesh(vertices);

    while (!this->window.should_close()) {
        this->window.poll_events();

        constexpr auto color = glm::vec4(0.2f, 0.3f, 0.3f, 1.0f);

        this->renderer->clear_color(color);
        this->renderer->clear(Luminol::Graphics::BufferBit::Color);

        this->renderer->draw(mesh->get_render_command(*this->renderer));

        this->window.swap_buffers();
    }
}

}  // namespace Luminol
