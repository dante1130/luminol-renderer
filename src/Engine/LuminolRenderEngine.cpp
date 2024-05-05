#include "LuminolRenderEngine.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <Engine/Graphics/Light.hpp>

namespace {

constexpr auto camera_initial_position = glm::vec3(5.0f, 0.0f, 0.0f);
constexpr auto camera_initial_forward = glm::vec3(-1.0f, 0.0f, 0.0f);
constexpr auto camera_rotation_speed = 0.1f;

}  // namespace

namespace Luminol {

RenderEngine::RenderEngine(const Properties& properties)
    : window(properties.width, properties.height, properties.title),
      camera(Graphics::CameraProperties{
          .position = camera_initial_position,
          .forward = camera_initial_forward,
          .rotation_speed = camera_rotation_speed
      }),
      graphics_factory(Graphics::GraphicsFactory::create(properties.graphics_api
      )),
      renderer(this->graphics_factory->create_renderer(this->window)) {}

auto RenderEngine::get_window() const -> const Window& { return this->window; }

auto RenderEngine::get_window() -> Window& { return this->window; }

auto RenderEngine::get_camera() const -> const Graphics::Camera& {
    return this->camera;
}

auto RenderEngine::get_camera() -> Graphics::Camera& { return this->camera; }

auto RenderEngine::get_graphics_factory() const
    -> const Graphics::GraphicsFactory& {
    return *this->graphics_factory;
}

auto RenderEngine::get_renderer() const -> const Graphics::Renderer& {
    return *this->renderer;
}

auto RenderEngine::get_renderer() -> Graphics::Renderer& {
    return *this->renderer;
}

}  // namespace Luminol
