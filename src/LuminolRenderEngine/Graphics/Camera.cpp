#include "Camera.hpp"

#include <algorithm>

#include <glm/gtc/matrix_transform.hpp>
#include <LuminolMaths/Transform.hpp>

namespace {

constexpr auto up_vector_world = Luminol::Maths::Vector3f{0.0f, 1.0f, 0.0f};

}  // namespace

namespace Luminol::Graphics {

Camera::Camera(const CameraProperties& properties)
    : position(properties.position),
      forward(properties.forward.normalized()),
      right_vector(this->forward.cross(up_vector_world).normalized()),
      up_vector(this->right_vector.cross(this->forward).normalized()),
      fov_degrees(properties.fov_degrees),
      aspect_ratio(properties.aspect_ratio),
      near_plane(properties.near_plane),
      far_plane(properties.far_plane),
      translation_speed(properties.translation_speed),
      rotation_speed(properties.rotation_speed),
      yaw_degrees(glm::degrees(std::atan2(this->forward.x(), this->forward.z()))
      ),
      pitch_degrees(glm::degrees(std::asin(this->forward.y()))) {}

auto Camera::move(CameraMovement direction, float delta_time_in_seconds)
    -> void {
    const auto velocity = this->translation_speed * delta_time_in_seconds;

    switch (direction) {
        case CameraMovement::Forward:
            this->position += this->forward * velocity;
            break;
        case CameraMovement::Backward:
            this->position -= this->forward * velocity;
            break;
        case CameraMovement::Left:
            this->position -= this->right_vector * velocity;
            break;
        case CameraMovement::Right:
            this->position += this->right_vector * velocity;
            break;
    }
}

auto Camera::rotate(float yaw_degrees, float pitch_degrees) -> void {
    this->yaw_degrees += yaw_degrees * this->rotation_speed;
    this->pitch_degrees += pitch_degrees * this->rotation_speed;

    constexpr auto max_yaw_degrees = 360.0f;
    constexpr auto max_pitch_degrees = 89.0f;
    constexpr auto min_pitch_degrees = -89.0f;

    this->yaw_degrees = std::fmod(this->yaw_degrees, max_yaw_degrees);
    this->pitch_degrees =
        std::clamp(this->pitch_degrees, min_pitch_degrees, max_pitch_degrees);

    const auto yaw_radians = glm::radians(this->yaw_degrees);
    const auto pitch_radians = glm::radians(this->pitch_degrees);

    this->forward.x() = std::sinf(yaw_radians) * std::cosf(pitch_radians);
    this->forward.y() = std::sinf(pitch_radians);
    this->forward.z() = std::cosf(yaw_radians) * std::cosf(pitch_radians);

    this->forward = this->forward.normalized();

    this->right_vector = up_vector_world.cross(this->forward).normalized();

    this->up_vector = this->forward.cross(this->right_vector).normalized();
}

auto Camera::get_view_matrix() const -> Maths::Matrix4x4f {
    return Maths::Transform::left_handed_look_at_matrix(
        Maths::Transform::LookAtParams<float>{
            .eye = this->position,
            .target = this->position + this->forward,
            .up_vector = up_vector_world
        }
    );
}

auto Camera::get_projection_matrix() const -> Maths::Matrix4x4f {
    return Maths::Transform::left_handed_perspective_projection_matrix(
        Maths::Transform::PerspectiveMatrixParams<float>{
            .fov = this->fov_degrees,
            .aspect_ratio = this->aspect_ratio,
            .near_plane = this->near_plane,
            .far_plane = this->far_plane
        }
    );
}

auto Camera::get_position() const -> const Maths::Vector3f& {
    return this->position;
}

auto Camera::get_forward() const -> const Maths::Vector3f& {
    return this->forward;
}

auto Camera::get_up_vector() const -> const Maths::Vector3f& {
    return this->up_vector;
}

auto Camera::get_right_vector() const -> const Maths::Vector3f& {
    return this->right_vector;
}

auto Camera::set_position(const Maths::Vector3f& position) -> void {
    this->position = position;
}

auto Camera::set_fov(float fov_degrees) -> void {
    this->fov_degrees = fov_degrees;
}

auto Camera::set_aspect_ratio(float aspect_ratio) -> void {
    this->aspect_ratio = aspect_ratio;
}

auto Camera::set_near_plane(float near_plane) -> void {
    this->near_plane = near_plane;
}

auto Camera::set_far_plane(float far_plane) -> void {
    this->far_plane = far_plane;
}

auto Camera::set_translation_speed(float translation_speed) -> void {
    this->translation_speed = translation_speed;
}

auto Camera::set_rotation_speed(float rotation_speed) -> void {
    this->rotation_speed = rotation_speed;
}

}  // namespace Luminol::Graphics
