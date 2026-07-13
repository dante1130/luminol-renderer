#include <cstdio>
#include <vector>

#include <LuminolMaths/Transform.hpp>
#include <LuminolRenderEngine/Graphics/Camera.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPURenderer.hpp>
#include <LuminolRenderEngine/LuminolRenderEngine.hpp>
#include <LuminolRenderEngine/Utilities/Timer.hpp>

// Headless stress test mirroring src/LuminolRenderEngine/Demo/InstancedCube's
// scene (grid_size=100 -> 1,000,000 cube instances) minus the interactive
// loop: drives SDL_GPURenderer::draw() for a fixed frame count and asserts
// the average frame time stays under a rough ceiling, to catch large
// regressions in instancing/indirect-draw/per-instance GPU culling at scale.
//
// THRESHOLD CALIBRATION: max_average_frame_time_ms below is a deliberately
// generous placeholder, not a measured baseline (this test can't be run in
// the environment that wrote it). Run this once, note the printed actual
// average, and tighten the threshold to ~2-3x that real number.

namespace {

using namespace Luminol;
using namespace Luminol::Graphics;

constexpr auto grid_size = 100;
constexpr auto grid_spacing = 5.0F;

constexpr auto warmup_frames = 30;
constexpr auto measured_frames = 120;

constexpr auto max_average_frame_time_ms = 12.0;

auto make_grid_model_matrices() -> std::vector<Maths::Matrix4x4f> {
    auto model_matrices = std::vector<Maths::Matrix4x4f>{};
    model_matrices.reserve(
        static_cast<size_t>(grid_size) * static_cast<size_t>(grid_size) *
        static_cast<size_t>(grid_size)
    );

    constexpr auto grid_offset =
        grid_spacing * static_cast<float>(grid_size - 1) / 2.0F;

    for (auto grid_x = 0; grid_x < grid_size; ++grid_x) {
        for (auto grid_y = 0; grid_y < grid_size; ++grid_y) {
            for (auto grid_z = 0; grid_z < grid_size; ++grid_z) {
                const auto position = Maths::Vector3f{
                    (static_cast<float>(grid_x) * grid_spacing) - grid_offset,
                    (static_cast<float>(grid_y) * grid_spacing) - grid_offset,
                    (static_cast<float>(grid_z) * grid_spacing) - grid_offset,
                };
                model_matrices.push_back(
                    Maths::Transform::translate_4x4(position)
                );
            }
        }
    }

    return model_matrices;
}

}  // namespace

auto main() -> int {
    using namespace Luminol;
    using namespace Luminol::Graphics;

    constexpr auto camera_initial_position =
        Maths::Vector3f{0.0F, 0.0F, -150.0F};
    constexpr auto camera_initial_forward = Maths::Vector3f{0.0F, 0.0F, 1.0F};
    constexpr auto camera_far_plane = 500.0F;

    auto luminol_engine = RenderEngine(Properties{
        .title = "Luminol Instanced Cube Stress Test",
    });

    auto camera = Camera{CameraProperties{
        .position = camera_initial_position,
        .forward = camera_initial_forward,
        .far_plane = camera_far_plane,
    }};

    const auto model_id =
        luminol_engine.get_renderer().create_renderable("res/models/cube/cube.obj");
    const auto model_matrices = make_grid_model_matrices();

    camera.set_aspect_ratio(
        static_cast<float>(luminol_engine.get_window().get_width()) /
        static_cast<float>(luminol_engine.get_window().get_height())
    );

    constexpr auto color = Maths::Vector4f{0.0F, 0.0F, 0.0F, 1.0F};

    auto run_frame = [&] {
        luminol_engine.get_renderer().clear_color(color);
        luminol_engine.get_renderer().set_view_matrix(camera.get_view_matrix());
        luminol_engine.get_renderer().set_projection_matrix(
            camera.get_projection_matrix()
        );
        luminol_engine.get_renderer().queue_draw_instanced(
            model_id, model_matrices
        );
        luminol_engine.get_renderer().draw();
    };

    for (auto frame = 0; frame < warmup_frames; ++frame) {
        run_frame();
    }

    auto total_frame_time_seconds = 0.0;
    auto worst_frame_time_seconds = 0.0;

    for (auto frame = 0; frame < measured_frames; ++frame) {
        auto timer = Utilities::Timer{};
        run_frame();
        const auto frame_time_seconds = timer.elapsed_seconds();

        total_frame_time_seconds += frame_time_seconds;
        if (frame_time_seconds > worst_frame_time_seconds) {
            worst_frame_time_seconds = frame_time_seconds;
        }
    }

    const auto average_frame_time_ms =
        (total_frame_time_seconds / measured_frames) * 1000.0;
    const auto worst_frame_time_ms = worst_frame_time_seconds * 1000.0;

    std::printf(
        "InstancedCube stress test: %d instances, %d frames measured "
        "(after %d warmup) - average %.3f ms/frame, worst %.3f ms/frame\n",
        static_cast<int>(model_matrices.size()),
        measured_frames,
        warmup_frames,
        average_frame_time_ms,
        worst_frame_time_ms
    );

    const auto success = average_frame_time_ms <= max_average_frame_time_ms;
    if (!success) {
        std::printf(
            "InstancedCube stress test FAILED: average %.3f ms/frame exceeds "
            "threshold %.3f ms/frame\n",
            average_frame_time_ms,
            max_average_frame_time_ms
        );
    } else {
        std::printf("InstancedCube stress test PASSED\n");
    }

    return success ? 0 : 1;
}
