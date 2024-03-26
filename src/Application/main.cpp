#include <Engine/Window/Window.hpp>
#include <Engine/Graphics/OpenGL/OpenGLRenderer.hpp>

auto main() -> int {
    constexpr auto width = 640;
    constexpr auto height = 480;
    constexpr auto opengl_version_major = 4;
    constexpr auto opengl_version_minor = 6;

    const auto window = Luminol::Window(
        width,
        height,
        "Application",
        Luminol::Window::WindowHints{
            .context_version_major = opengl_version_major,
            .context_version_minor = opengl_version_minor,
        }
    );

    const auto renderer = Luminol::Graphics::OpenGLRenderer(window);

    while (!window.should_close()) {
        window.poll_events();

        constexpr auto color = glm::vec4(0.2f, 0.3f, 0.3f, 1.0f);

        renderer.clear_color(color);
        renderer.clear(Luminol::Graphics::BufferBit::Color);

        window.swap_buffers();
    }

    return 0;
}
