#include <iostream>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

auto main() -> int {
    if (glfwInit() == GLFW_FALSE) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    constexpr auto width = 640;
    constexpr auto height = 480;

    auto* window =
        glfwCreateWindow(width, height, "Application", nullptr, nullptr);

    if (window == nullptr) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    const auto version = gladLoadGL(glfwGetProcAddress);
    if (version == 0) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    std::cout << "OpenGL Version " << GLAD_VERSION_MAJOR(version) << "."
              << GLAD_VERSION_MINOR(version) << " loaded\n";

    while (glfwWindowShouldClose(window) == GLFW_FALSE) {
        glfwPollEvents();

        struct Color {
            float r = 0.0f;
            float g = 0.0f;
            float b = 0.0f;
            float a = 0.0f;
        };

        constexpr Color color{0.2f, 0.3f, 0.3f, 1.0f};

        glClearColor(color.r, color.g, color.b, color.a);
        glClear(GL_COLOR_BUFFER_BIT);

        glfwSwapBuffers(window);
    }

    glfwTerminate();

    return 0;
}
