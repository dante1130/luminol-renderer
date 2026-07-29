#include "SphereMesh.hpp"

#include <cmath>
#include <numbers>

namespace Luminol::Demo::PbrSpheres {

auto make_uv_sphere(
    float radius, uint32_t latitude_segments, uint32_t longitude_segments
) -> SphereMesh {
    constexpr auto vertex_stride_in_floats = 11U;
    constexpr auto indices_per_quad = 6U;
    constexpr auto tangent_epsilon = 1e-6F;
    constexpr auto pi_value = std::numbers::pi_v<float>;

    auto mesh = SphereMesh{};

    const auto verts_per_ring = longitude_segments + 1U;
    mesh.vertices.reserve(
        static_cast<size_t>(latitude_segments + 1U) * verts_per_ring *
        vertex_stride_in_floats
    );

    for (auto lat = 0U; lat <= latitude_segments; ++lat) {
        const auto theta = (static_cast<float>(lat) * pi_value) /
                            static_cast<float>(latitude_segments);
        const auto sin_theta = std::sin(theta);
        const auto cos_theta = std::cos(theta);

        for (auto lon = 0U; lon <= longitude_segments; ++lon) {
            const auto phi = (static_cast<float>(lon) * 2.0F * pi_value) /
                              static_cast<float>(longitude_segments);
            const auto sin_phi = std::sin(phi);
            const auto cos_phi = std::cos(phi);

            const auto normal_x = sin_theta * cos_phi;
            const auto normal_y = cos_theta;
            const auto normal_z = sin_theta * sin_phi;

            const auto position_x = radius * normal_x;
            const auto position_y = radius * normal_y;
            const auto position_z = radius * normal_z;

            const auto uv_u =
                static_cast<float>(lon) / static_cast<float>(longitude_segments);
            const auto uv_v =
                static_cast<float>(lat) / static_cast<float>(latitude_segments);

            // Derivative of position w.r.t. increasing phi (increasing u).
            // Degenerates to zero at the poles (sin_theta == 0); fall back to
            // a fixed axis there since exact tangent handedness isn't
            // load-bearing (the shader re-orthogonalizes against the normal
            // and derives the bitangent itself - see pbr_frag.hlsl).
            auto tangent_x = -sin_theta * sin_phi;
            auto tangent_y = 0.0F;
            auto tangent_z = sin_theta * cos_phi;
            const auto tangent_length = std::sqrt(
                (tangent_x * tangent_x) + (tangent_y * tangent_y) +
                (tangent_z * tangent_z)
            );
            if (tangent_length < tangent_epsilon) {
                tangent_x = 1.0F;
                tangent_y = 0.0F;
                tangent_z = 0.0F;
            } else {
                tangent_x /= tangent_length;
                tangent_y /= tangent_length;
                tangent_z /= tangent_length;
            }

            mesh.vertices.insert(
                mesh.vertices.end(),
                {position_x, position_y, position_z, uv_u, uv_v, normal_x,
                 normal_y, normal_z, tangent_x, tangent_y, tangent_z}
            );
        }
    }

    mesh.indices.reserve(
        static_cast<size_t>(latitude_segments) * longitude_segments *
        indices_per_quad
    );

    for (auto lat = 0U; lat < latitude_segments; ++lat) {
        for (auto lon = 0U; lon < longitude_segments; ++lon) {
            const auto top_left = (lat * verts_per_ring) + lon;
            const auto top_right = top_left + 1U;
            const auto bottom_left = top_left + verts_per_ring;
            const auto bottom_right = bottom_left + 1U;

            mesh.indices.insert(
                mesh.indices.end(),
                {top_left, top_right, bottom_left, top_right, bottom_right,
                 bottom_left}
            );
        }
    }

    return mesh;
}

}  // namespace Luminol::Demo::PbrSpheres
