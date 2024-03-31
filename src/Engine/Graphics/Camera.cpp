#include "Camera.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace Luminol::Graphics {

Camera::Camera(const CameraProperties& properties) : properties(properties) {}

auto Camera::move(CameraMovement direction, float delta_time_in_seconds)
    -> void {
    const auto velocity =
        this->properties.translation_speed * delta_time_in_seconds;

    switch (direction) {
        case CameraMovement::Forward:
            this->properties.position += this->properties.forward * velocity;
            break;
        case CameraMovement::Backward:
            this->properties.position -= this->properties.forward * velocity;
            break;
        case CameraMovement::Left:
            this->properties.position -= this->right_vector * velocity;
            break;
        case CameraMovement::Right:
            this->properties.position += this->right_vector * velocity;
            break;
    }
}

auto Camera::get_view_matrix() const -> glm::mat4 {
    constexpr auto up_vector = glm::vec3{0.0f, 1.0f, 0.0f};

    return glm::lookAtLH(
        this->properties.position,
        this->properties.position + this->properties.forward,
        up_vector
    );
}

auto Camera::get_projection_matrix() const -> glm::mat4 {
    return glm::perspectiveLH(
        glm::radians(this->properties.fov_degrees),
        this->properties.aspect_ratio,
        this->properties.near_plane,
        this->properties.far_plane
    );
}

auto Camera::get_position() const -> const glm::vec3& {
    return this->properties.position;
}

auto Camera::get_forward() const -> const glm::vec3& {
    return this->properties.forward;
}

auto Camera::get_up_vector() const -> const glm::vec3& {
    return this->properties.up_vector;
}

auto Camera::get_right_vector() const -> const glm::vec3& {
    return this->right_vector;
}

auto Camera::set_position(const glm::vec3& position) -> void {
    this->properties.position = position;
}

auto Camera::set_fov(float fov_degrees) -> void {
    this->properties.fov_degrees = fov_degrees;
}

auto Camera::set_aspect_ratio(float aspect_ratio) -> void {
    this->properties.aspect_ratio = aspect_ratio;
}

auto Camera::set_near_plane(float near_plane) -> void {
    this->properties.near_plane = near_plane;
}

auto Camera::set_far_plane(float far_plane) -> void {
    this->properties.far_plane = far_plane;
}

auto Camera::set_translation_speed(float translation_speed) -> void {
    this->properties.translation_speed = translation_speed;
}

auto Camera::set_rotation_speed(float rotation_speed) -> void {
    this->properties.rotation_speed = rotation_speed;
}

}  // namespace Luminol::Graphics
