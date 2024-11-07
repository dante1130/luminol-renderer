#include "Camera.hpp"

#include <algorithm>

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
      fov(properties.fov),
      aspect_ratio(properties.aspect_ratio),
      near_plane(properties.near_plane),
      far_plane(properties.far_plane),
      translation_speed(properties.translation_speed),
      rotation_speed(properties.rotation_speed),
      yaw(Units::Radians_f{std::atan2(this->forward.x(), this->forward.z())}),
      pitch(Units::Radians_f{std::asin(this->forward.y())}) {}

auto Camera::move(CameraMovement direction, Units::Seconds_f delta_time)
    -> void {
    const auto velocity = this->translation_speed * delta_time.get_value();

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

auto Camera::rotate(Units::Degrees_f yaw, Units::Degrees_f pitch) -> void {
    this->yaw += yaw * this->rotation_speed;
    this->pitch += pitch * this->rotation_speed;

    constexpr auto max_yaw_degrees = Units::Degrees_f{360.0f};
    constexpr auto max_pitch_degrees = Units::Degrees_f{89.0f};
    constexpr auto min_pitch_degrees = Units::Degrees_f{-89.0f};

    this->yaw = std::fmod(this->yaw.get_value(), max_yaw_degrees.get_value());
    this->pitch = std::clamp(
        this->pitch.get_value(),
        min_pitch_degrees.get_value(),
        max_pitch_degrees.get_value()
    );

    const auto yaw_radians = this->yaw.as<Units::Radian>();
    const auto pitch_radians = this->pitch.as<Units::Radian>();

    this->forward.x() = std::sinf(yaw_radians.get_value()) *
                        std::cosf(pitch_radians.get_value());
    this->forward.y() = std::sinf(pitch_radians.get_value());
    this->forward.z() = std::cosf(yaw_radians.get_value()) *
                        std::cosf(pitch_radians.get_value());

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
            .fov = this->fov,
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

auto Camera::set_fov(Units::Degrees_f fov) -> void { this->fov = fov; }

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
