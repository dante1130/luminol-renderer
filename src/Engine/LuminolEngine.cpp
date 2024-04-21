#include "LuminolEngine.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace {

constexpr auto camera_initial_position = glm::vec3(5.0f, 0.0f, 0.0f);
constexpr auto camera_initial_forward = glm::vec3(-1.0f, 0.0f, 0.0f);
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
    constexpr auto exposure = 2.0f;
    this->renderer->set_exposure(exposure);

    const auto model =
        this->graphics_factory->create_model("res/models/mech_drone/scene.gltf"
        );

    const auto cube =
        this->graphics_factory->create_model("res/models/cube/cube.obj");

    constexpr auto directional_light = Graphics::DirectionalLight{
        .direction = glm::vec3(0.5f, -0.5f, 1.0f),
        .ambient = glm::vec3(0.2f, 0.2f, 0.2f),
        .diffuse = glm::vec3(1.0f, 1.0f, 1.0f),
        .specular = glm::vec3(2.0f, 2.0f, 2.0f)
    };

    this->renderer->get_light_manager().update_directional_light(
        directional_light
    );

    constexpr auto point_light_1_position = glm::vec3(0.0f, 1.0f, 2.0f);
    constexpr auto point_light_1_color = glm::vec3(1.0f, 0.0f, 0.0f);

    {
        constexpr auto point_light = Graphics::PointLight{
            .position = point_light_1_position,
            .ambient = glm::vec3(0.2f, 0.2f, 0.2f),
            .diffuse = glm::vec3(1.0f, 0.0f, 0.0f),
            .specular = glm::vec3(2.0f, 2.0f, 2.0f),
            .constant = 1.0f,
            .linear = 0.09f,
            .quadratic = 0.032f
        };

        const auto point_light_id_opt =
            this->renderer->get_light_manager().add_point_light(point_light);

        if (!point_light_id_opt.has_value()) {
            throw std::runtime_error("Failed to add point light");
        }

        const auto point_light_id = point_light_id_opt.value();

        this->renderer->get_light_manager().update_point_light(
            point_light_id, point_light
        );
    }

    constexpr auto point_light_2_position = glm::vec3(0.0f, 2.0f, 0.0f);
    constexpr auto point_light_2_color = glm::vec3(0.0f, 1.0f, 0.0f);

    {
        constexpr auto point_light = Graphics::PointLight{
            .position = point_light_2_position,
            .ambient = glm::vec3(0.2f, 0.2f, 0.2f),
            .diffuse = glm::vec3(0.0f, 1.0f, 0.0f),
            .specular = glm::vec3(2.0f, 2.0f, 2.0f),
            .constant = 1.0f,
            .linear = 0.09f,
            .quadratic = 0.032f
        };

        const auto point_light_id_opt =
            this->renderer->get_light_manager().add_point_light(point_light);

        if (!point_light_id_opt.has_value()) {
            throw std::runtime_error("Failed to add point light");
        }

        const auto point_light_id = point_light_id_opt.value();

        this->renderer->get_light_manager().update_point_light(
            point_light_id, point_light
        );
    }

    const auto initial_flash_light = Graphics::SpotLight{
        .position = this->camera.get_position(),
        .direction = this->camera.get_forward(),
        .ambient = glm::vec3(0.0f, 0.0f, 0.0f),
        .diffuse = glm::vec3(1.0f, 1.0f, 1.0f),
        .specular = glm::vec3(2.0f, 2.0f, 2.0f),
        .constant = 1.0f,
        .linear = 0.09f,
        .quadratic = 0.032f,
        .cut_off = glm::cos(glm::radians(12.5f)),
        .outer_cut_off = glm::cos(glm::radians(17.5f))
    };

    auto flash_light = initial_flash_light;

    const auto flash_light_id_opt =
        this->renderer->get_light_manager().add_spot_light(flash_light);

    if (!flash_light_id_opt.has_value()) {
        throw std::runtime_error("Failed to add spot light");
    }

    const auto flash_light_id = flash_light_id_opt.value();

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
        this->renderer->clear(Graphics::BufferBit::ColorDepth);

        this->camera.set_aspect_ratio(
            static_cast<float>(this->window.get_width()) /
            static_cast<float>(this->window.get_height())
        );

        this->renderer->set_view_matrix(this->camera.get_view_matrix());
        this->renderer->set_projection_matrix(
            this->camera.get_projection_matrix()
        );

        flash_light.position = this->camera.get_position();
        flash_light.direction = this->camera.get_forward();

        this->renderer->get_light_manager().update_spot_light(
            flash_light_id, flash_light
        );

        {
            constexpr auto scale = glm::vec3(0.1f, 0.1f, 0.1f);

            auto model_matrix = glm::mat4(1.0f);
            model_matrix = glm::translate(model_matrix, point_light_1_position);
            model_matrix = glm::scale(model_matrix, scale);

            this->renderer->queue_draw_with_color(
                *cube, model_matrix, point_light_1_color
            );
        }

        {
            constexpr auto scale = glm::vec3(0.1f, 0.1f, 0.1f);

            auto model_matrix = glm::mat4(1.0f);
            model_matrix = glm::translate(model_matrix, point_light_2_position);
            model_matrix = glm::scale(model_matrix, scale);

            this->renderer->queue_draw_with_color(
                *cube, model_matrix, point_light_2_color
            );
        }

        {
            constexpr auto scale = glm::vec3(5.0f, 5.0f, 5.0f);
            constexpr auto rotation_degrees = 180.0f;

            auto model_matrix = glm::mat4(1.0f);
            model_matrix = glm::scale(model_matrix, scale);
            model_matrix = glm::rotate(
                model_matrix,
                glm::radians(rotation_degrees),
                glm::vec3(0.0f, 1.0f, 0.0f)
            );

            this->renderer->queue_draw_with_phong(*model, model_matrix);
        }

        this->renderer->draw();

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
