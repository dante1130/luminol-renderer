#include <cmath>
#include <numbers>

#include <LuminolMaths/Units/Angle.hpp>
#include <LuminolMaths/Units/Time.hpp>
#include <LuminolMaths/Vector.hpp>

#include <LuminolRenderEngine/Graphics/Camera.hpp>

#include <doctest/doctest.h>

using namespace Luminol;
using namespace Luminol::Graphics;

namespace {

auto make_camera() -> Camera {
    return Camera{CameraProperties{
        .position = Maths::Vector3f{0.0F, 0.0F, 0.0F},
        .forward = Maths::Vector3f{0.0F, 0.0F, 1.0F},
        .up_vector = Maths::Vector3f{0.0F, 1.0F, 0.0F},
        .fov = Units::Degrees_f{45.0F},
        .aspect_ratio = 1.0F,
        .near_plane = 0.1F,
        .far_plane = 100.0F,
        .translation_speed = 1.0F,
        .rotation_speed = 1.0F,
    }};
}

}  // namespace

TEST_CASE("rotate clamps pitch to [-89, 89] degrees") {
    constexpr auto max_pitch_radians = 89.0F * std::numbers::pi_v<float> / 180.0F;

    auto camera = make_camera();

    // Well past +89 degrees - should clamp to exactly +89, not pass through.
    camera.rotate(Units::Degrees_f{0.0F}, Units::Degrees_f{200.0F});
    CHECK(camera.get_forward().y() == doctest::Approx(std::sin(max_pitch_radians)).epsilon(0.001));

    // From +89, well past -89 degrees - should clamp to exactly -89.
    camera.rotate(Units::Degrees_f{0.0F}, Units::Degrees_f{-400.0F});
    CHECK(camera.get_forward().y() == doctest::Approx(-std::sin(max_pitch_radians)).epsilon(0.001));
}

TEST_CASE("rotate keeps the forward vector unit length") {
    auto camera = make_camera();

    camera.rotate(Units::Degrees_f{37.0F}, Units::Degrees_f{15.0F});

    CHECK(camera.get_forward().length() == doctest::Approx(1.0F).epsilon(0.001));
}

TEST_CASE("rotate wraps yaw and does not accumulate unbounded") {
    auto camera = make_camera();

    for (auto i = 0; i < 100; ++i) {
        camera.rotate(Units::Degrees_f{50.0F}, Units::Degrees_f{0.0F});
    }

    CHECK(camera.get_forward().length() == doctest::Approx(1.0F).epsilon(0.001));
}

TEST_CASE("move Forward displaces position along the forward vector") {
    auto camera = make_camera();

    camera.move(CameraMovement::Forward, Units::Seconds_f{2.0F});

    const auto& position = camera.get_position();
    CHECK(position.x() == doctest::Approx(0.0F).epsilon(0.001));
    CHECK(position.y() == doctest::Approx(0.0F).epsilon(0.001));
    CHECK(position.z() == doctest::Approx(2.0F).epsilon(0.001));
}

TEST_CASE("move Backward displaces position opposite the forward vector") {
    auto camera = make_camera();

    camera.move(CameraMovement::Backward, Units::Seconds_f{2.0F});

    CHECK(camera.get_position().z() == doctest::Approx(-2.0F).epsilon(0.001));
}

TEST_CASE("move Right displaces position along the right vector") {
    auto camera = make_camera();

    camera.move(CameraMovement::Right, Units::Seconds_f{1.0F});

    const auto& position = camera.get_position();
    const auto& right = camera.get_right_vector();
    CHECK(position.x() == doctest::Approx(right.x()).epsilon(0.001));
    CHECK(position.y() == doctest::Approx(right.y()).epsilon(0.001));
    CHECK(position.z() == doctest::Approx(right.z()).epsilon(0.001));
}

TEST_CASE("move Left displaces position opposite the right vector") {
    auto camera = make_camera();

    camera.move(CameraMovement::Left, Units::Seconds_f{1.0F});

    const auto& position = camera.get_position();
    const auto& right = camera.get_right_vector();
    CHECK(position.x() == doctest::Approx(-right.x()).epsilon(0.001));
    CHECK(position.y() == doctest::Approx(-right.y()).epsilon(0.001));
    CHECK(position.z() == doctest::Approx(-right.z()).epsilon(0.001));
}

TEST_CASE("constructor back-computes yaw/pitch so a zero rotate leaves forward unchanged"
) {
    auto camera = Camera{CameraProperties{
        .position = Maths::Vector3f{0.0F, 0.0F, 0.0F},
        .forward = Maths::Vector3f{1.0F, 0.0F, 0.0F},
        .up_vector = Maths::Vector3f{0.0F, 1.0F, 0.0F},
    }};

    const auto forward_before = camera.get_forward();
    camera.rotate(Units::Degrees_f{0.0F}, Units::Degrees_f{0.0F});
    const auto& forward_after = camera.get_forward();

    CHECK(forward_after.x() == doctest::Approx(forward_before.x()).epsilon(0.001));
    CHECK(forward_after.y() == doctest::Approx(forward_before.y()).epsilon(0.001));
    CHECK(forward_after.z() == doctest::Approx(forward_before.z()).epsilon(0.001));
}
