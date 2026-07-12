#include <LuminolMaths/Transform.hpp>
#include <LuminolMaths/Units/Angle.hpp>
#include <LuminolMaths/Vector.hpp>

#include <LuminolRenderEngine/Graphics/Frustum.hpp>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

namespace {

using Luminol::Graphics::aabb_in_frustum;
using Luminol::Graphics::extract_frustum_corners;
using Luminol::Graphics::extract_frustum_planes;
using Luminol::Graphics::sphere_in_frustum;
using Luminol::Maths::Vector3f;

// Camera at the origin looking down +Z, 90 degree vertical FOV, near 0.1,
// far 100 - matches the view/projection combination and axis conventions
// used throughout the renderer (see SDL_GPURenderer.cpp: view * projection).
auto make_test_view_projection() -> Luminol::Maths::Matrix4x4f {
    const auto view = Luminol::Maths::Transform::left_handed_look_at_matrix(
        Luminol::Maths::Transform::LookAtParams<float>{
            .eye = Vector3f{0.0F, 0.0F, 0.0F},
            .target = Vector3f{0.0F, 0.0F, 1.0F},
            .up_vector = Vector3f{0.0F, 1.0F, 0.0F},
        }
    );

    const auto projection =
        Luminol::Maths::Transform::left_handed_perspective_projection_matrix(
            Luminol::Maths::Transform::PerspectiveMatrixParams<float>{
                .fov = Luminol::Units::Degrees_f{90.0F},
                .aspect_ratio = 1.0F,
                .near_plane = 0.1F,
                .far_plane = 100.0F,
            }
        );

    return view * projection;
}

}  // namespace

TEST_CASE("sphere_in_frustum accepts a sphere inside the frustum") {
    const auto view_projection = make_test_view_projection();
    const auto planes = extract_frustum_planes(view_projection);

    CHECK(sphere_in_frustum(planes, Vector3f{0.0F, 0.0F, 5.0F}, 1.0F));
}

TEST_CASE("sphere_in_frustum rejects a sphere far to the side") {
    const auto view_projection = make_test_view_projection();
    const auto planes = extract_frustum_planes(view_projection);

    CHECK_FALSE(sphere_in_frustum(planes, Vector3f{100.0F, 0.0F, 5.0F}, 1.0F));
}

TEST_CASE("sphere_in_frustum rejects a sphere behind the camera") {
    const auto view_projection = make_test_view_projection();
    const auto planes = extract_frustum_planes(view_projection);

    CHECK_FALSE(sphere_in_frustum(planes, Vector3f{0.0F, 0.0F, -5.0F}, 1.0F));
}

TEST_CASE("aabb_in_frustum accepts a box inside the frustum") {
    const auto view_projection = make_test_view_projection();
    const auto planes = extract_frustum_planes(view_projection);

    CHECK(aabb_in_frustum(
        planes, Vector3f{-0.5F, -0.5F, 4.5F}, Vector3f{0.5F, 0.5F, 5.5F}
    ));
}

TEST_CASE("aabb_in_frustum rejects a box entirely behind the camera") {
    const auto view_projection = make_test_view_projection();
    const auto planes = extract_frustum_planes(view_projection);

    CHECK_FALSE(aabb_in_frustum(
        planes, Vector3f{-0.5F, -0.5F, -5.5F}, Vector3f{0.5F, 0.5F, -4.5F}
    ));
}

TEST_CASE("extract_frustum_corners places near corners closer than far corners"
) {
    const auto view_projection = make_test_view_projection();
    const auto corners = extract_frustum_corners(view_projection);

    // Order: near-bottom-left, near-bottom-right, near-top-left,
    // near-top-right, far-bottom-left, far-bottom-right, far-top-left,
    // far-top-right.
    for (auto i = size_t{0}; i < 4; ++i) {
        CHECK(corners.at(i).z() < corners.at(i + 4).z());
    }
}
