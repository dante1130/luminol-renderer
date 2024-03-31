#include "LuminolEngine.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace {

constexpr auto camera_initial_position = glm::vec3(0.0f, 0.0f, -3.0f);
constexpr auto camera_initial_forward = glm::vec3(0.0f, 0.0f, 1.0f);
constexpr auto camera_rotation_speed = 0.1f;

}  // namespace

namespace Luminol {

Engine::Engine(const Properties& properties)
    : window(properties.width, properties.height, properties.title),
      camera(Graphics::CameraProperties{
          .position = camera_initial_position,
          .forward = camera_initial_forward,
          .rotation_speed = camera_rotation_speed
      }),
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
        const double current_frame_time_seconds = this->timer.elapsed_seconds();
        this->delta_time_seconds =
            current_frame_time_seconds - this->last_frame_time_seconds;
        this->last_frame_time_seconds = current_frame_time_seconds;

        this->window.poll_events();

        this->handle_key_events();

        const auto mouse_delta = this->window.get_mouse_delta();
        this->camera.rotate(
            gsl::narrow_cast<float>(mouse_delta.delta_x),
            gsl::narrow_cast<float>(mouse_delta.delta_y)
        );

        constexpr auto color = glm::vec4(0.2f, 0.3f, 0.3f, 1.0f);

        this->renderer->clear_color(color);
        this->renderer->clear(Graphics::BufferBit::Color);

        this->camera.set_aspect_ratio(
            static_cast<float>(this->window.get_width()) /
            static_cast<float>(this->window.get_height())
        );

        this->renderer->set_view_matrix(this->camera.get_view_matrix());
        this->renderer->set_projection_matrix(
            this->camera.get_projection_matrix()
        );

        constexpr auto rotation_degrees_x = 20.0f;

        auto model_matrix = glm::mat4(1.0f);
        model_matrix = glm::rotate(
            model_matrix,
            glm::radians(rotation_degrees_x),
            glm::vec3(1.0f, 0.0f, 0.0f)
        );

        this->renderer->draw(
            mesh->get_render_command(*this->renderer), model_matrix
        );

        this->window.swap_buffers();
    }
}

auto Engine::handle_key_events() -> void {
    const auto delta_time_seconds_f =
        gsl::narrow_cast<float>(this->delta_time_seconds);

    if (this->window.is_key_event('W', KeyEvent::Press)) {
        this->camera.move(
            Graphics::CameraMovement::Forward, delta_time_seconds_f
        );
    }

    if (this->window.is_key_event('S', KeyEvent::Press)) {
        this->camera.move(
            Graphics::CameraMovement::Backward, delta_time_seconds_f
        );
    }

    if (this->window.is_key_event('A', KeyEvent::Press)) {
        this->camera.move(Graphics::CameraMovement::Left, delta_time_seconds_f);
    }

    if (this->window.is_key_event('D', KeyEvent::Press)) {
        this->camera.move(
            Graphics::CameraMovement::Right, delta_time_seconds_f
        );
    }

    if (this->window.is_key_event('Q', KeyEvent::Press)) {
        this->window.close();
    }
}

}  // namespace Luminol
