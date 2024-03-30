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

    while (!this->window.should_close()) {
        this->window.poll_events();

        constexpr auto color = glm::vec4(0.2f, 0.3f, 0.3f, 1.0f);

        this->renderer->clear_color(color);
        this->renderer->clear(Graphics::BufferBit::Color);

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
