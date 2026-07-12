#include "Frustum.hpp"

#include <cmath>

namespace Luminol::Graphics {

auto extract_frustum_planes(const Maths::Matrix4x4f& view_projection)
    -> std::array<Maths::Vector4f, 6> {
    const auto column = [&view_projection](int index) {
        return Maths::Vector4f{
            view_projection[0][index],
            view_projection[1][index],
            view_projection[2][index],
            view_projection[3][index],
        };
    };

    const auto col0 = column(0);
    const auto col1 = column(1);
    const auto col2 = column(2);
    const auto col3 = column(3);

    return std::array<Maths::Vector4f, 6>{
        col3 + col0,  // left
        col3 - col0,  // right
        col3 + col1,  // bottom
        col3 - col1,  // top
        col2,         // near
        col3 - col2,  // far
    };
}

auto extract_frustum_corners(const Maths::Matrix4x4f& view_projection)
    -> std::array<Maths::Vector3f, 8> {
    const auto inverse_view_projection = view_projection.inverse();

    constexpr auto ndc_x = std::array{-1.0F, 1.0F};
    constexpr auto ndc_y = std::array{-1.0F, 1.0F};
    constexpr auto ndc_z = std::array{0.0F, 1.0F};

    auto corners = std::array<Maths::Vector3f, 8>{};
    auto index = size_t{0};

    for (const auto z : ndc_z) {
        for (const auto y : ndc_y) {
            for (const auto x : ndc_x) {
                const auto ndc_position = Maths::Vector4f{x, y, z, 1.0F};

                auto world_position = Maths::Vector4f{};
                for (size_t column = 0; column < 4; ++column) {
                    auto sum = 0.0F;
                    for (size_t row = 0; row < 4; ++row) {
                        sum += ndc_position[row] *
                            inverse_view_projection[row][column];
                    }
                    world_position[column] = sum;
                }

                corners.at(index) = Maths::Vector3f{
                    world_position.x() / world_position.w(),
                    world_position.y() / world_position.w(),
                    world_position.z() / world_position.w(),
                };
                ++index;
            }
        }
    }

    return corners;
}

auto sphere_in_frustum(
    const std::array<Maths::Vector4f, 6>& planes,
    const Maths::Vector3f& center,
    float radius
) -> bool {
    for (const auto& plane : planes) {
        const auto normal_length = std::sqrt(
            (plane.x() * plane.x()) + (plane.y() * plane.y()) +
            (plane.z() * plane.z())
        );
        if (normal_length < 1e-6F) {
            continue;
        }

        const auto distance =
            ((plane.x() * center.x()) + (plane.y() * center.y()) +
             (plane.z() * center.z()) + plane.w()) /
            normal_length;

        if (distance < -radius) {
            return false;
        }
    }

    return true;
}

auto aabb_in_frustum(
    const std::array<Maths::Vector4f, 6>& planes,
    const Maths::Vector3f& box_min,
    const Maths::Vector3f& box_max
) -> bool {
    for (const auto& plane : planes) {
        const auto positive_vertex = Maths::Vector3f{
            plane.x() >= 0.0F ? box_max.x() : box_min.x(),
            plane.y() >= 0.0F ? box_max.y() : box_min.y(),
            plane.z() >= 0.0F ? box_max.z() : box_min.z(),
        };

        const auto distance = (plane.x() * positive_vertex.x()) +
            (plane.y() * positive_vertex.y()) +
            (plane.z() * positive_vertex.z()) + plane.w();

        if (distance < 0.0F) {
            return false;
        }
    }

    return true;
}

}  // namespace Luminol::Graphics
