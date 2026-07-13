#include <algorithm>
#include <array>
#include <cstdio>

#include <LuminolMaths/Transform.hpp>
#include <LuminolRenderEngine/Graphics/Camera.hpp>
#include <LuminolRenderEngine/Graphics/Light.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPURenderer.hpp>
#include <LuminolRenderEngine/LuminolRenderEngine.hpp>
#include <LuminolRenderEngine/Utilities/Timer.hpp>

// Headless stress test exercising LightManager::update_shadow_casters'
// per-frame scoring/hysteresis and the clustered light-culling GPU pass at
// their designed capacity ceiling (max_point_lights / max_spot_lights, 1024
// each - Light.hpp). Scene geometry is the Sponza model (same asset/camera
// framing as Demo/Sponza) plus a single small cube, so this also stresses
// per-submesh batch culling/shading cost alongside the light-culling cost.
//
// THRESHOLD CALIBRATION: max_average_frame_time_ms below is a deliberately
// generous placeholder, not a measured baseline (this test can't be run in
// the environment that wrote it). Run this once, note the printed actual
// average, and tighten the threshold to ~2-3x that real number.

namespace {

using namespace Luminol;
using namespace Luminol::Graphics;

// 8x8x16 = 1024, matching max_point_lights/max_spot_lights exactly.
constexpr auto grid_x = 8;
constexpr auto grid_y = 8;
constexpr auto grid_z = 16;
constexpr auto grid_spacing = 2.0F;

constexpr auto warmup_frames = 30;
constexpr auto measured_frames = 120;

constexpr auto max_average_frame_time_ms = 7.0;

auto add_lights(LightManager& light_manager) -> void {
    // Centered near the origin, roughly Sponza's interior volume (see
    // Demo/Sponza's camera position/forward below) - not an exact fit to the
    // mesh bounds, just close enough that lights land inside the scene
    // rather than off in empty space.
    constexpr auto offset_x = grid_spacing * static_cast<float>(grid_x - 1) / 2.0F;
    constexpr auto offset_z = grid_spacing * static_cast<float>(grid_z - 1) / 2.0F;

    for (auto x = 0; x < grid_x; ++x) {
        for (auto y = 0; y < grid_y; ++y) {
            for (auto z = 0; z < grid_z; ++z) {
                const auto position = Maths::Vector3f{
                    (static_cast<float>(x) * grid_spacing) - offset_x,
                    1.0F + (static_cast<float>(y) * grid_spacing),
                    (static_cast<float>(z) * grid_spacing) - offset_z,
                };

                (void)light_manager.add_point_light(PointLight{
                    .position = position,
                    .color = Maths::Vector3f{1.0F, 1.0F, 1.0F},
                });

                (void)light_manager.add_spot_light(SpotLight{
                    .position = position,
                    .direction = Maths::Vector3f{0.0F, -1.0F, 0.0F},
                    .color = Maths::Vector3f{1.0F, 1.0F, 1.0F},
                    .cut_off = 0.9F,
                    .outer_cut_off = 0.8F,
                });
            }
        }
    }
}

}  // namespace

auto main() -> int {
    using namespace Luminol;
    using namespace Luminol::Graphics;

    // Same camera framing as Demo/Sponza.
    constexpr auto camera_initial_position = Maths::Vector3f{10.0F, 1.0F, 0.0F};
    constexpr auto camera_initial_forward = Maths::Vector3f{-1.0F, 0.0F, 0.0F};
    constexpr auto camera_far_plane = 200.0F;

    auto luminol_engine = RenderEngine(Properties{
        .title = "Luminol Many Lights Stress Test",
    });

    auto camera = Camera{CameraProperties{
        .position = camera_initial_position,
        .forward = camera_initial_forward,
        .far_plane = camera_far_plane,
    }};

    const auto sponza_model_id = luminol_engine.get_renderer().create_renderable(
        "res/models/Sponza/glTF/Sponza.gltf"
    );

    add_lights(luminol_engine.get_renderer().get_light_manager());

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
        luminol_engine.get_renderer().queue_draw(
            sponza_model_id, Maths::Matrix4x4f::identity()
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
        worst_frame_time_seconds =
            std::max(worst_frame_time_seconds, frame_time_seconds);
    }

    const auto average_frame_time_ms =
        (total_frame_time_seconds / measured_frames) * 1000.0;
    const auto worst_frame_time_ms = worst_frame_time_seconds * 1000.0;

    std::printf(
        "ManyLights stress test: %d point + %d spot lights, %d frames "
        "measured (after %d warmup) - average %.3f ms/frame, worst %.3f "
        "ms/frame\n",
        grid_x * grid_y * grid_z,
        grid_x * grid_y * grid_z,
        measured_frames,
        warmup_frames,
        average_frame_time_ms,
        worst_frame_time_ms
    );

    const auto success = average_frame_time_ms <= max_average_frame_time_ms;
    if (!success) {
        std::printf(
            "ManyLights stress test FAILED: average %.3f ms/frame exceeds "
            "threshold %.3f ms/frame\n",
            average_frame_time_ms,
            max_average_frame_time_ms
        );
    } else {
        std::printf("ManyLights stress test PASSED\n");
    }

    return success ? 0 : 1;
}
