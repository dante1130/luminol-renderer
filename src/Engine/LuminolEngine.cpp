#include "LuminolEngine.hpp"

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

auto Engine::get_window() const -> const Window& {
    return this->window;
}

auto Engine::get_window() -> Window& {
    return this->window;
}

auto Engine::get_camera() const -> const Graphics::Camera& {
    return this->camera;
}

auto Engine::get_camera() -> Graphics::Camera& {
    return this->camera;
}

auto Engine::get_graphics_factory() const -> const Graphics::GraphicsFactory& {
    return *this->graphics_factory;
}

auto Engine::get_renderer() const -> const Graphics::Renderer& {
    return *this->renderer;
}

auto Engine::get_renderer() -> Graphics::Renderer& {
    return *this->renderer;
}

auto Engine::get_delta_time_seconds() const -> double {
    return this->delta_time_seconds;
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
