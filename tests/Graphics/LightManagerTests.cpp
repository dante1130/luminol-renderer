#include <cstdint>
#include <optional>

#include <LuminolMaths/Transform.hpp>
#include <LuminolMaths/Units/Angle.hpp>
#include <LuminolMaths/Vector.hpp>

#include <LuminolRenderEngine/Graphics/Light.hpp>
#include <LuminolRenderEngine/Graphics/LightManager.hpp>

#include <doctest/doctest.h>

using namespace Luminol::Graphics;
using namespace Luminol::Maths;

namespace {

// Camera at the origin looking down +Z - matches the convention used by
// FrustumTests.cpp/ClusterLightCullSmokeTest's main.cpp.
auto make_test_view_projection() -> Matrix4x4f {
    const auto view = Transform::left_handed_look_at_matrix(
        Transform::LookAtParams<float>{
            .eye = Vector3f{0.0F, 0.0F, 0.0F},
            .target = Vector3f{0.0F, 0.0F, 1.0F},
            .up_vector = Vector3f{0.0F, 1.0F, 0.0F},
        }
    );

    const auto projection = Transform::left_handed_perspective_projection_matrix(
        Transform::PerspectiveMatrixParams<float>{
            .fov = Luminol::Units::Degrees_f{90.0F},
            .aspect_ratio = 1.0F,
            .near_plane = 0.1F,
            .far_plane = 100.0F,
        }
    );

    return view * projection;
}

// A point light on the camera's forward axis at the given distance, white
// (color {1,1,1}), so light_cull_radius is the same for every light created
// this way and ranking is purely a function of distance.
auto light_at_z(float z) -> PointLight {
    return PointLight{
        .position = Vector3f{0.0F, 0.0F, z},
        .color = Vector3f{1.0F, 1.0F, 1.0F},
    };
}

}  // namespace

TEST_CASE("add_point_light returns nullopt once max_point_lights is exhausted") {
    auto light_manager = LightManager{};

    for (auto i = 0u; i < max_point_lights; ++i) {
        const auto id = light_manager.add_point_light(light_at_z(5.0F));
        REQUIRE(id.has_value());
    }

    CHECK(light_manager.add_point_light(light_at_z(5.0F)) == std::nullopt);
}

TEST_CASE("removed point light ids are reused, lowest id first") {
    auto light_manager = LightManager{};

    const auto id0 = light_manager.add_point_light(light_at_z(5.0F));
    const auto id1 = light_manager.add_point_light(light_at_z(6.0F));
    REQUIRE(id0.has_value());
    REQUIRE(id1.has_value());

    light_manager.remove_point_light(*id0);

    const auto id2 = light_manager.add_point_light(light_at_z(7.0F));
    REQUIRE(id2.has_value());
    CHECK(*id2 == *id0);
}

TEST_CASE("update_shadow_casters never assigns a slot to a light behind the camera") {
    auto light_manager = LightManager{};

    const auto id = light_manager.add_point_light(light_at_z(-5.0F));
    REQUIRE(id.has_value());

    light_manager.update_shadow_casters(
        make_test_view_projection(), Vector3f{0.0F, 0.0F, 0.0F}
    );

    const auto& light_data = light_manager.get_light_data();
    CHECK(light_data.point_lights[0].shadow_data.x() == doctest::Approx(-1.0F));
}

TEST_CASE(
    "update_shadow_casters assigns slots to exactly the nearest "
    "max_shadow_casting_point_lights and no others"
) {
    auto light_manager = LightManager{};

    // One light per slot plus enough extras to spill well past the
    // max_slots*1.25 hysteresis soft margin, so ranking is unambiguous:
    // ids are added nearest-first, so id == array index into point_lights
    // (get_light_data() iterates in ascending LightId order).
    constexpr auto extra_lights = 10u;
    for (auto i = 0u; i < max_shadow_casting_point_lights + extra_lights; ++i) {
        const auto id =
            light_manager.add_point_light(light_at_z(5.0F + static_cast<float>(i)));
        REQUIRE(id.has_value());
        REQUIRE(*id == i);
    }

    light_manager.update_shadow_casters(
        make_test_view_projection(), Vector3f{0.0F, 0.0F, 0.0F}
    );

    const auto& light_data = light_manager.get_light_data();

    for (auto i = 0u; i < max_shadow_casting_point_lights; ++i) {
        CAPTURE(i);
        const auto slot = light_data.point_lights[i].shadow_data.x();
        CHECK(slot >= 0.0F);
        CHECK(slot < static_cast<float>(max_shadow_casting_point_lights));
    }

    // The farthest lights are well outside the soft margin
    // (ceil(max_slots*1.25)), so they must never get a slot.
    for (auto i = max_shadow_casting_point_lights;
         i < max_shadow_casting_point_lights + extra_lights;
         ++i) {
        CAPTURE(i);
        CHECK(
            light_data.point_lights[i].shadow_data.x() == doctest::Approx(-1.0F)
        );
    }
}

TEST_CASE(
    "update_shadow_casters is stable across frames when nothing changes"
) {
    auto light_manager = LightManager{};

    for (auto i = 0u; i < max_shadow_casting_point_lights + 1; ++i) {
        const auto id =
            light_manager.add_point_light(light_at_z(5.0F + static_cast<float>(i)));
        REQUIRE(id.has_value());
    }

    const auto view_projection = make_test_view_projection();
    const auto camera_position = Vector3f{0.0F, 0.0F, 0.0F};

    light_manager.update_shadow_casters(view_projection, camera_position);
    auto first_pass_slots = std::array<float, max_shadow_casting_point_lights + 1>{};
    {
        const auto& light_data = light_manager.get_light_data();
        for (auto i = 0u; i < max_shadow_casting_point_lights + 1; ++i) {
            first_pass_slots.at(i) = light_data.point_lights[i].shadow_data.x();
        }
    }

    light_manager.update_shadow_casters(view_projection, camera_position);
    const auto& light_data = light_manager.get_light_data();
    for (auto i = 0u; i < max_shadow_casting_point_lights + 1; ++i) {
        CAPTURE(i);
        CHECK(light_data.point_lights[i].shadow_data.x() == first_pass_slots.at(i));
    }
}

TEST_CASE(
    "update_shadow_casters keeps a still-strict light's slot number when "
    "ranking is reshuffled"
) {
    auto light_manager = LightManager{};

    for (auto i = 0u; i < max_shadow_casting_point_lights; ++i) {
        const auto id =
            light_manager.add_point_light(light_at_z(5.0F + static_cast<float>(i)));
        REQUIRE(id.has_value());
    }

    const auto view_projection = make_test_view_projection();
    const auto camera_position = Vector3f{0.0F, 0.0F, 0.0F};

    light_manager.update_shadow_casters(view_projection, camera_position);
    float slot_of_id0 = 0;
    float slot_of_id5 = 0;
    {
        const auto& light_data = light_manager.get_light_data();
        slot_of_id0 = light_data.point_lights[0].shadow_data.x();
        slot_of_id5 = light_data.point_lights[5].shadow_data.x();
    }

    // Swap id0 and id5's distances - both stay comfortably within the
    // strict top max_shadow_casting_point_lights, just in a different order.
    light_manager.update_point_light(0, light_at_z(10.0F));
    light_manager.update_point_light(5, light_at_z(5.0F));

    light_manager.update_shadow_casters(view_projection, camera_position);
    const auto& light_data = light_manager.get_light_data();

    CHECK(light_data.point_lights[0].shadow_data.x() == slot_of_id0);
    CHECK(light_data.point_lights[5].shadow_data.x() == slot_of_id5);
}
