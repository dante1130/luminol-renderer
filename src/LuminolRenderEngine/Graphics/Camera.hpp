#pragma once

#include <LuminolMaths/Vector.hpp>
#include <LuminolMaths/Matrix.hpp>
#include <LuminolMaths/Units/Time.hpp>
#include <LuminolMaths/Units/Angle.hpp>

namespace Luminol::Graphics {

enum class CameraMovement {
    Forward,
    Backward,
    Left,
    Right,
};

constexpr static auto default_fov = Units::Degrees_f{45.0f};
constexpr static auto default_aspect_ratio = 1.0f;
constexpr static auto default_near_plane = 0.1f;
constexpr static auto default_far_plane = 100.0f;

struct CameraProperties {
    Maths::Vector3f position = {0.0f, 0.0f, 0.0f};
    Maths::Vector3f forward = {0.0f, 0.0f, 1.0f};
    Maths::Vector3f up_vector = {0.0f, 1.0f, 0.0f};

    Units::Degrees_f fov = default_fov;
    float aspect_ratio = default_aspect_ratio;
    float near_plane = default_near_plane;
    float far_plane = default_far_plane;

    float translation_speed = 1.0f;
    float rotation_speed = 1.0f;
};

class Camera {
public:
    Camera(const CameraProperties& properties);

    auto move(CameraMovement direction, Units::Seconds_f delta_time) -> void;
    auto rotate(Units::Degrees_f yaw, Units::Degrees_f pitch) -> void;

    [[nodiscard]] auto get_view_matrix() const -> Maths::Matrix4x4f;
    [[nodiscard]] auto get_projection_matrix() const -> Maths::Matrix4x4f;

    [[nodiscard]] auto get_position() const -> const Maths::Vector3f&;
    [[nodiscard]] auto get_forward() const -> const Maths::Vector3f&;
    [[nodiscard]] auto get_up_vector() const -> const Maths::Vector3f&;
    [[nodiscard]] auto get_right_vector() const -> const Maths::Vector3f&;

    auto set_position(const Maths::Vector3f& position) -> void;
    auto set_fov(Units::Degrees_f fov) -> void;
    auto set_aspect_ratio(float aspect_ratio) -> void;
    auto set_near_plane(float near_plane) -> void;
    auto set_far_plane(float far_plane) -> void;
    auto set_translation_speed(float translation_speed) -> void;
    auto set_rotation_speed(float rotation_speed) -> void;

private:
    Maths::Vector3f position = {0.0f, 0.0f, 0.0f};
    Maths::Vector3f forward = {0.0f, 0.0f, 1.0f};
    Maths::Vector3f right_vector = {1.0f, 0.0f, 0.0f};
    Maths::Vector3f up_vector = {0.0f, 1.0f, 0.0f};

    Units::Degrees_f fov = default_fov;
    float aspect_ratio = default_aspect_ratio;
    float near_plane = default_near_plane;
    float far_plane = default_far_plane;

    float translation_speed = 1.0f;
    float rotation_speed = 1.0f;

    Units::Degrees_f yaw = 0.0f;
    Units::Degrees_f pitch = 0.0f;
};

}  // namespace Luminol::Graphics
