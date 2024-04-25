#include "LuminolEngine.hpp"

#include <random>

#include <glm/gtc/matrix_transform.hpp>

#include <Engine/Graphics/Light.hpp>

namespace {

constexpr auto camera_initial_position = glm::vec3(5.0f, 0.0f, 0.0f);
constexpr auto camera_initial_forward = glm::vec3(-1.0f, 0.0f, 0.0f);
constexpr auto camera_rotation_speed = 0.1f;

using namespace Luminol::Graphics;

struct LightEntity {
    glm::vec3 position;
    glm::vec3 color;
    std::unique_ptr<Model> model;
};

auto create_point_light(const glm::vec3& position, const glm::vec3& color)
    -> PointLight {
    constexpr auto specular_multiplier = 2.0f;
    constexpr auto constant = 0.0f;
    constexpr auto linear = 1.0f;
    constexpr auto quadratic = 0.0f;

    return PointLight{
        .position = position,
        .ambient = glm::vec3(0.0f),
        .diffuse = color,
        .specular = color * specular_multiplier,
        .constant = constant,
        .linear = linear,
        .quadratic = quadratic
    };
}

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
        .ambient = glm::vec3(0.0f, 0.0f, 0.0f),
        .diffuse = glm::vec3(0.0f, 0.0f, 0.0f),
        .specular = glm::vec3(0.0f, 0.0f, 0.0f)
    };

    this->renderer->get_light_manager().update_directional_light(
        directional_light
    );

    constexpr auto lights_count = 64u;

    auto entities = std::vector<LightEntity>{};
    entities.reserve(lights_count);

    auto random = std::mt19937{std::random_device{}()};

    for (auto i = 0u; i < lights_count; ++i) {
        const auto position = glm::vec3(
            std::uniform_real_distribution<float>(-5.0f, 5.0f)(random),
            std::uniform_real_distribution<float>(-5.0f, 5.0f)(random),
            std::uniform_real_distribution<float>(-5.0f, 5.0f)(random)
        );

        const auto color = glm::vec3(
            std::uniform_real_distribution<float>(0.0f, 1.0f)(random),
            std::uniform_real_distribution<float>(0.0f, 1.0f)(random),
            std::uniform_real_distribution<float>(0.0f, 1.0f)(random)
        );

        entities.emplace_back(
            position,
            color,
            this->graphics_factory->create_model("res/models/cube/cube.obj")
        );

        const auto point_light_id_opt =
            this->renderer->get_light_manager().add_point_light(
                create_point_light(position, color)
            );

        if (!point_light_id_opt.has_value()) {
            throw std::runtime_error("Failed to add point light");
        }
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

        constexpr auto color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

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

        for (const auto& entity : entities) {
            constexpr auto scale = glm::vec3(0.1f, 0.1f, 0.1f);

            auto model_matrix = glm::mat4(1.0f);
            model_matrix = glm::translate(model_matrix, entity.position);
            model_matrix = glm::scale(model_matrix, scale);

            this->renderer->queue_draw_with_color(
                *entity.model, model_matrix, entity.color
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

            this->renderer->queue_draw(*model, model_matrix);
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
