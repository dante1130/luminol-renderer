#pragma once

#include <array>

#include <LuminolMaths/Matrix.hpp>
#include <LuminolMaths/Vector.hpp>

namespace Luminol::Graphics {

// Gribb/Hartmann frustum plane extraction for a row-major view-projection
// matrix used with row-vector multiplication (mul(pos, vp), see
// pbr_vert.hlsl), so each plane's coefficients come from a column of the
// matrix rather than a row. Returned order: left, right, bottom, top, near,
// far.
[[nodiscard]] auto extract_frustum_planes(
    const Maths::Matrix4x4f& view_projection
) -> std::array<Maths::Vector4f, 6>;

// Unprojects the 8 NDC cube corners (x/y in [-1, 1], z in [0, 1], matching
// the D3D-style depth range used throughout this codebase - see
// left_handed_perspective_projection_matrix) through the inverse of a
// row-major view-projection matrix used with row-vector multiplication
// (pos * view_projection, see pbr_vert.hlsl), returning their world-space
// positions. Order: near-bottom-left, near-bottom-right, near-top-left,
// near-top-right, far-bottom-left, far-bottom-right, far-top-left,
// far-top-right.
[[nodiscard]] auto extract_frustum_corners(
    const Maths::Matrix4x4f& view_projection
) -> std::array<Maths::Vector3f, 8>;

[[nodiscard]] auto sphere_in_frustum(
    const std::array<Maths::Vector4f, 6>& planes,
    const Maths::Vector3f& center,
    float radius
) -> bool;

// Positive-vertex AABB-vs-frustum test: for each plane, the box corner most
// aligned with the plane's normal is picked; if even that corner is behind
// the plane, the whole box is outside it. Tighter than sphere_in_frustum for
// large flat/elongated boxes.
[[nodiscard]] auto aabb_in_frustum(
    const std::array<Maths::Vector4f, 6>& planes,
    const Maths::Vector3f& box_min,
    const Maths::Vector3f& box_max
) -> bool;

}  // namespace Luminol::Graphics
