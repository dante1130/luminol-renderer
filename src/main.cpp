#include <iostream>

#include <glad/gl.h>
#include <glm/glm.hpp>

#include <Window.hpp>

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

    const auto version = gladLoadGL(Luminol::Window::get_proc_address());
    if (version == 0) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    std::cout << "OpenGL Version " << GLAD_VERSION_MAJOR(version) << "."
              << GLAD_VERSION_MINOR(version) << " loaded\n";

    while (!window.should_close()) {
        Luminol::Window::poll_events();

        constexpr auto color = glm::vec4(0.2f, 0.3f, 0.3f, 1.0f);

        glClearColor(color.r, color.g, color.b, color.a);
        glClear(GL_COLOR_BUFFER_BIT);

        window.swap_buffers();
    }

    return 0;
}
