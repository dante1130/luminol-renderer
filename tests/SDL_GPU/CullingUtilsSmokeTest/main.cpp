#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <memory>
#include <vector>

#include <gsl/gsl>
#include <SDL3/SDL_video.h>

#include <LuminolMaths/Matrix.hpp>
#include <LuminolMaths/Transform.hpp>
#include <LuminolMaths/Vector.hpp>

#include <LuminolRenderEngine/Graphics/BoundingBox.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCommandBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCopyPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCullingUtils.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUDevice.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUMesh.hpp>
#include <LuminolRenderEngine/Window/Window.hpp>

// Validates SDL_GPUCullingUtils::compute_mesh_world_bounds by independently
// recomputing the expected world-space AABB (transform the local bounds'
// corners by each model matrix, take the union min/max) in plain C++ and
// comparing against the function's own (already-CPU-side) output. The mesh
// itself only needs a bare GPUDevice - compute_mesh_world_bounds is pure CPU
// math over SDL_GPUMesh::get_local_bounds(), no compute dispatch or readback
// involved.

namespace {

using namespace Luminol::Graphics;
using namespace Luminol::Graphics::SDL_GPU;
using namespace Luminol::Maths;

constexpr auto epsilon = 0.001F;

auto approx_equal(float lhs, float rhs) -> bool {
    return std::abs(lhs - rhs) <= epsilon;
}

auto bounds_approx_equal(const BoundingBox& lhs, const BoundingBox& rhs) -> bool {
    return approx_equal(lhs.min.x(), rhs.min.x()) &&
        approx_equal(lhs.min.y(), rhs.min.y()) &&
        approx_equal(lhs.min.z(), rhs.min.z()) &&
        approx_equal(lhs.max.x(), rhs.max.x()) &&
        approx_equal(lhs.max.y(), rhs.max.y()) &&
        approx_equal(lhs.max.z(), rhs.max.z());
}

// Mirrors SDL_GPUCullingUtils.cpp's own transform_point (row-vector
// convention, mul(pos, matrix)).
auto transform_point(const Matrix4x4f& matrix, const Vector3f& point) -> Vector3f {
    return Vector3f{
        (point.x() * matrix[0][0]) + (point.y() * matrix[1][0]) +
            (point.z() * matrix[2][0]) + matrix[3][0],
        (point.x() * matrix[0][1]) + (point.y() * matrix[1][1]) +
            (point.z() * matrix[2][1]) + matrix[3][1],
        (point.x() * matrix[0][2]) + (point.y() * matrix[1][2]) +
            (point.z() * matrix[2][2]) + matrix[3][2],
    };
}

auto expected_world_bounds(
    const BoundingBox& local_bounds, const std::vector<Matrix4x4f>& model_matrices
) -> BoundingBox {
    if (model_matrices.empty()) {
        return local_bounds;
    }

    const auto corners = std::array<Vector3f, 8>{
        Vector3f{local_bounds.min.x(), local_bounds.min.y(), local_bounds.min.z()},
        Vector3f{local_bounds.max.x(), local_bounds.min.y(), local_bounds.min.z()},
        Vector3f{local_bounds.min.x(), local_bounds.max.y(), local_bounds.min.z()},
        Vector3f{local_bounds.max.x(), local_bounds.max.y(), local_bounds.min.z()},
        Vector3f{local_bounds.min.x(), local_bounds.min.y(), local_bounds.max.z()},
        Vector3f{local_bounds.max.x(), local_bounds.min.y(), local_bounds.max.z()},
        Vector3f{local_bounds.min.x(), local_bounds.max.y(), local_bounds.max.z()},
        Vector3f{local_bounds.max.x(), local_bounds.max.y(), local_bounds.max.z()},
    };

    auto world_min = Vector3f{};
    auto world_max = Vector3f{};
    auto initialized = false;

    for (const auto& matrix : model_matrices) {
        for (const auto& corner : corners) {
            const auto world_corner = transform_point(matrix, corner);

            if (!initialized) {
                world_min = world_corner;
                world_max = world_corner;
                initialized = true;
                continue;
            }

            world_min = Vector3f{
                std::min(world_min.x(), world_corner.x()),
                std::min(world_min.y(), world_corner.y()),
                std::min(world_min.z(), world_corner.z()),
            };
            world_max = Vector3f{
                std::max(world_max.x(), world_corner.x()),
                std::max(world_max.y(), world_corner.y()),
                std::max(world_max.z(), world_corner.z()),
            };
        }
    }

    return BoundingBox{.min = world_min, .max = world_max};
}

struct TestCase {
    const char* name;
    std::vector<Matrix4x4f> model_matrices;
};

}  // namespace

auto main() -> int {
    using namespace Luminol;
    using namespace Luminol::Graphics;
    using namespace Luminol::Graphics::SDL_GPU;
    using namespace Luminol::Maths;

    auto window = Window{1, 1, "Luminol Culling Utils Smoke Test"};
    auto gpu_device = std::make_shared<GPUDevice>(
        static_cast<SDL_Window*>(window.get_window_handle())
    );

    const auto local_bounds =
        BoundingBox{.min = Vector3f{-1.0F, -1.0F, -1.0F}, .max = Vector3f{1.0F, 1.0F, 1.0F}};

    auto command_buffer = gpu_device->create_command_buffer();
    const auto mesh = [&] {
        auto copy_pass = command_buffer.begin_copy_pass();
        return SDL_GPUMesh{
            *gpu_device, copy_pass, 0, 36, 0, local_bounds, TextureImages{}
        };
    }();
    command_buffer.submit();
    gpu_device->wait_for_idle();

    const auto rotation_y =
        Transform::rotate_y<float, 4>(Units::Degrees_f{37.0F}.as<Units::Radian>());
    const auto rotation_x =
        Transform::rotate_x<float, 4>(Units::Degrees_f{21.0F}.as<Units::Radian>());
    const auto rotation = rotation_y * rotation_x;
    const auto non_uniform_scale =
        Transform::scale_4x4(Vector3f{1.0F, 2.0F, 3.0F});
    const auto translation =
        Transform::translate_4x4(Vector3f{5.0F, -3.0F, 8.0F});

    const auto test_cases = std::array<TestCase, 8>{
        TestCase{
            .name = "identity",
            .model_matrices = {Matrix4x4f::identity()},
        },
        TestCase{
            .name = "translation",
            .model_matrices = {Transform::translate_4x4(Vector3f{5.0F, 0.0F, 0.0F})},
        },
        TestCase{
            .name = "scale",
            .model_matrices = {Transform::scale_4x4(Vector3f{2.0F, 2.0F, 2.0F})},
        },
        TestCase{
            .name = "union of two instances",
            .model_matrices =
                {Matrix4x4f::identity(),
                 Transform::translate_4x4(Vector3f{10.0F, 0.0F, 0.0F})},
        },
        TestCase{
            .name = "empty span",
            .model_matrices = {},
        },
        TestCase{
            .name = "rotation",
            .model_matrices = {rotation},
        },
        TestCase{
            .name = "non-uniform scale",
            .model_matrices = {non_uniform_scale},
        },
        TestCase{
            .name = "rotation + non-uniform scale + translation combined",
            .model_matrices = {rotation * non_uniform_scale * translation},
        },
    };

    auto success = true;
    for (const auto& test_case : test_cases) {
        const auto actual =
            compute_mesh_world_bounds(mesh, gsl::span{test_case.model_matrices});
        const auto expected =
            expected_world_bounds(local_bounds, test_case.model_matrices);

        if (!bounds_approx_equal(actual, expected)) {
            std::printf(
                "Culling utils smoke test FAILED at case '%s': "
                "expected min(%f,%f,%f) max(%f,%f,%f), got min(%f,%f,%f) max(%f,%f,%f)\n",
                test_case.name,
                expected.min.x(),
                expected.min.y(),
                expected.min.z(),
                expected.max.x(),
                expected.max.y(),
                expected.max.z(),
                actual.min.x(),
                actual.min.y(),
                actual.min.z(),
                actual.max.x(),
                actual.max.y(),
                actual.max.z()
            );
            success = false;
        }
    }

    if (success) {
        std::printf(
            "Culling utils smoke test PASSED (%zu cases checked)\n",
            test_cases.size()
        );
    }

    return success ? 0 : 1;
}
