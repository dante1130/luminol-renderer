#include <cstdio>
#include <vector>

#include <LuminolMaths/Transform.hpp>
#include <LuminolRenderEngine/Graphics/Camera.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPURenderer.hpp>
#include <LuminolRenderEngine/LuminolRenderEngine.hpp>
#include <LuminolRenderEngine/Utilities/Timer.hpp>

// Headless stress test placing the camera close to a dense, wide/tall grid
// of cubes filling the whole viewport, so nearly every screen pixel is close
// to geometry. SDL_GPUScreenSpaceReflectionPass runs unconditionally every
// frame (as part of SDL_GPURenderer::draw's record_ao_and_ssr step) and
// marches up to a fixed max step count per pixel; a grazing, geometry-dense
// view like this maximizes the number of pixels whose rays march the full
// distance before hitting/missing something, which none of the other
// stress tests' camera framings (distant or looking through sparse space)
// trigger.
//
// THRESHOLD CALIBRATION: max_average_frame_time_ms below is a deliberately
// generous placeholder, not a measured baseline (this test can't be run in
// the environment that wrote it). Run this once, note the printed actual
// average, and tighten the threshold to ~2-3x that real number.

namespace {

using namespace Luminol;
using namespace Luminol::Graphics;

constexpr auto grid_width = 60;
constexpr auto grid_height = 40;
constexpr auto grid_depth = 20;
constexpr auto grid_spacing = 1.5F;

constexpr auto warmup_frames = 30;
constexpr auto measured_frames = 120;

constexpr auto max_average_frame_time_ms = 7.0;

auto make_wall_model_matrices() -> std::vector<Maths::Matrix4x4f> {
    auto model_matrices = std::vector<Maths::Matrix4x4f>{};
    model_matrices.reserve(
        static_cast<size_t>(grid_width) * static_cast<size_t>(grid_height) *
        static_cast<size_t>(grid_depth)
    );

    constexpr auto offset_x = grid_spacing * static_cast<float>(grid_width - 1) / 2.0F;
    constexpr auto offset_y = grid_spacing * static_cast<float>(grid_height - 1) / 2.0F;

    for (auto grid_x = 0; grid_x < grid_width; ++grid_x) {
        for (auto grid_y = 0; grid_y < grid_height; ++grid_y) {
            for (auto grid_z = 0; grid_z < grid_depth; ++grid_z) {
                const auto position = Maths::Vector3f{
                    (static_cast<float>(grid_x) * grid_spacing) - offset_x,
                    (static_cast<float>(grid_y) * grid_spacing) - offset_y,
                    static_cast<float>(grid_z) * grid_spacing,
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

    // Positioned just in front of the near face of the cube wall, looking
    // straight into it, so the viewport is filled edge-to-edge with close
    // geometry.
    constexpr auto camera_initial_position = Maths::Vector3f{0.0F, 0.0F, -3.0F};
    constexpr auto camera_initial_forward = Maths::Vector3f{0.0F, 0.0F, 1.0F};
    constexpr auto camera_far_plane = 100.0F;

    auto luminol_engine = RenderEngine(Properties{
        .title = "Luminol Screen Space Reflection Stress Test",
    });

    auto camera = Camera{CameraProperties{
        .position = camera_initial_position,
        .forward = camera_initial_forward,
        .far_plane = camera_far_plane,
    }};

    const auto model_id =
        luminol_engine.get_renderer().create_renderable("res/models/cube/cube.obj");
    const auto model_matrices = make_wall_model_matrices();

    luminol_engine.get_renderer().queue_draw_instanced_static(
        model_id, model_matrices
    );

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
        "ScreenSpaceReflection stress test: %d instances, %d frames "
        "measured (after %d warmup) - average %.3f ms/frame, worst %.3f "
        "ms/frame\n",
        static_cast<int>(model_matrices.size()),
        measured_frames,
        warmup_frames,
        average_frame_time_ms,
        worst_frame_time_ms
    );

    const auto success = average_frame_time_ms <= max_average_frame_time_ms;
    if (!success) {
        std::printf(
            "ScreenSpaceReflection stress test FAILED: average %.3f "
            "ms/frame exceeds threshold %.3f ms/frame\n",
            average_frame_time_ms,
            max_average_frame_time_ms
        );
    } else {
        std::printf("ScreenSpaceReflection stress test PASSED\n");
    }

    return success ? 0 : 1;
}
