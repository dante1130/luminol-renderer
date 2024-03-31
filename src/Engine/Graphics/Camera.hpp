#pragma once

#include <glm/glm.hpp>

namespace Luminol::Graphics {

enum class CameraMovement {
    Forward,
    Backward,
    Left,
    Right,
};

constexpr static auto default_fov_degrees = 45.0f;
constexpr static auto default_aspect_ratio = 1.0f;
constexpr static auto default_near_plane = 0.1f;
constexpr static auto default_far_plane = 100.0f;

struct CameraProperties {
    glm::vec3 position = {0.0f, 0.0f, 0.0f};
    glm::vec3 forward = {0.0f, 0.0f, 1.0f};
    glm::vec3 up_vector = {0.0f, 1.0f, 0.0f};

    float fov_degrees = default_fov_degrees;
    float aspect_ratio = default_aspect_ratio;
    float near_plane = default_near_plane;
    float far_plane = default_far_plane;

    float translation_speed = 1.0f;
    float rotation_speed = 1.0f;
};

class Camera {
public:
    Camera(const CameraProperties& properties);

    auto move(CameraMovement direction, float delta_time) -> void;
    auto rotate(float yaw_degrees, float pitch_degrees) -> void;

    [[nodiscard]] auto get_view_matrix() const -> glm::mat4;
    [[nodiscard]] auto get_projection_matrix() const -> glm::mat4;

    [[nodiscard]] auto get_position() const -> const glm::vec3&;
    [[nodiscard]] auto get_forward() const -> const glm::vec3&;
    [[nodiscard]] auto get_up_vector() const -> const glm::vec3&;
    [[nodiscard]] auto get_right_vector() const -> const glm::vec3&;

    auto set_position(const glm::vec3& position) -> void;
    auto set_fov(float fov_degrees) -> void;
    auto set_aspect_ratio(float aspect_ratio) -> void;
    auto set_near_plane(float near_plane) -> void;
    auto set_far_plane(float far_plane) -> void;
    auto set_translation_speed(float translation_speed) -> void;
    auto set_rotation_speed(float rotation_speed) -> void;

private:
    CameraProperties properties;
    glm::vec3 right_vector = {1.0f, 0.0f, 0.0f};

    float yaw_degrees = 0.0f;
    float pitch_degrees = 0.0f;
};

}  // namespace Luminol::Graphics
