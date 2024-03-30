#include "LuminolEngine.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace Luminol {

Engine::Engine(const Properties& properties)
    : window(properties.width, properties.height, properties.title),
      graphics_factory(Graphics::GraphicsFactory::create(properties.graphics_api
      )),
      renderer(this->graphics_factory->create_renderer(this->window)) {}

void Engine::run() {
    constexpr auto vertices = std::array{
        0.5f,  0.5f,  0.0f, 1.0f, 1.0f, 0.5f,  -0.5f, 0.0f, 1.0f, 0.0f,
        -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, -0.5f, 0.5f,  0.0f, 0.0f, 1.0f,
    };

    constexpr auto indices = std::array{
        0u,
        1u,
        3u,  // first triangle
        1u,
        2u,
        3u  // second triangle
    };

    const auto mesh = this->graphics_factory->create_mesh(
        vertices, indices, "res/textures/reflex.png"
    );

    constexpr auto camera_position = glm::vec3{0.0f, 0.0f, -3.0f};
    constexpr auto camera_target = glm::vec3{0.0f, 0.0f, 1.0f};
    constexpr auto camera_up = glm::vec3{0.0f, 1.0f, 0.0f};

    const auto view_matrix =
        glm::lookAt(camera_position, camera_target, camera_up);

    while (!this->window.should_close()) {
        this->window.poll_events();

        constexpr auto color = glm::vec4(0.2f, 0.3f, 0.3f, 1.0f);

        this->renderer->clear_color(color);
        this->renderer->clear(Graphics::BufferBit::Color);

        constexpr auto fov_degrees = 45.0f;
        constexpr auto near_plane = 0.1f;
        constexpr auto far_plane = 100.0f;

        const auto perspective_matrix = glm::perspective(
            glm::radians(fov_degrees),
            static_cast<float>(this->window.get_width()) /
                static_cast<float>(this->window.get_height()),
            near_plane,
            far_plane
        );

        this->renderer->set_view_matrix(view_matrix);
        this->renderer->set_projection_matrix(perspective_matrix);

        constexpr auto rotation_degrees_x = 20.0f;
        constexpr auto scale = glm::vec3(2.0f, 2.0f, 2.0f);

        auto model_matrix = glm::mat4(1.0f);
        model_matrix = glm::rotate(
            model_matrix,
            glm::radians(rotation_degrees_x),
            glm::vec3(1.0f, 0.0f, 0.0f)
        );
        model_matrix = glm::scale(model_matrix, scale);

        this->renderer->draw(
            mesh->get_render_command(*this->renderer), model_matrix
        );

        this->window.swap_buffers();
    }
}

}  // namespace Luminol
