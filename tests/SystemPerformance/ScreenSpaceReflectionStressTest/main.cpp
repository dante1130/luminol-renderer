#include <array>
#include <cstdint>
#include <cstdio>
#include <vector>

#include <gsl/gsl>

#include <LuminolMaths/Transform.hpp>
#include <LuminolMaths/Vector.hpp>
#include <LuminolRenderEngine/Graphics/Camera.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPURenderer.hpp>
#include <LuminolRenderEngine/Graphics/TexturePaths.hpp>
#include <LuminolRenderEngine/LuminolRenderEngine.hpp>
#include <LuminolRenderEngine/Utilities/Timer.hpp>

// Headless stress test for SDL_GPUScreenSpaceReflectionPass (runs
// unconditionally every frame as part of SDL_GPURenderer::draw's
// record_ao_and_ssr step). Scene is a large, low-roughness/metallic floor
// quad viewed from above at a shallow/grazing angle with open sky above it -
// the actual SSR worst case, since reflection rays traced from a surface
// with nothing nearby to hit have to search close to the full
// max_distance/max_steps budget before giving up, unlike rays that
// immediately hit adjacent close geometry.
//
// The floor uses the same reflective-quad technique as
// Demo/TexturedModel/main.cpp (res/textures/floor_albedo.png +
// floor_metallic_roughness.png: metallic from the blue channel, roughness
// from green) rather than res/models/cube/cube.obj. cube.obj has no
// metallic/roughness map, so it falls back to a fully metallic AND fully
// rough (roughness = 1.0) material - pbr_frag.hlsl only samples the SSR
// result when roughness < 0.95 (see its ssr_max_roughness fade), so a
// cube.obj floor traced zero visible reflections despite the SSR pass
// itself running at full cost. A small cluster of cubes is placed above the
// floor, in view, purely so there's visible geometry for the floor to
// reflect (confirms the pass is actually doing something, not just
// stressing march cost on an empty scene).
//
// THRESHOLD CALIBRATION: max_average_frame_time_ms below is a deliberately
// generous placeholder, not a measured baseline (this test can't be run in
// the environment that wrote it). Run this once, note the printed actual
// average, and tighten the threshold to ~2-3x that real number.

namespace {

using namespace Luminol;
using namespace Luminol::Graphics;

constexpr auto floor_half_size = 100.0F;
constexpr auto floor_uv_tiles = 50.0F;
constexpr auto floor_y = -1.0F;

// Vertex layout: position.xyz, uv.xy, normal.xyz, tangent.xyz (11 floats),
// matching Demo/TexturedModel's procedural floor quad.
constexpr auto floor_vertices = std::array<float, size_t{11} * 4>{
    -floor_half_size, floor_y, -floor_half_size,
    0.0F, 0.0F,   0.0F, 1.0F, 0.0F,   1.0F, 0.0F, 0.0F,
    -floor_half_size, floor_y, floor_half_size,
    0.0F, floor_uv_tiles,   0.0F, 1.0F, 0.0F,   1.0F, 0.0F, 0.0F,
    floor_half_size, floor_y, floor_half_size,
    floor_uv_tiles, floor_uv_tiles,   0.0F, 1.0F, 0.0F,   1.0F, 0.0F, 0.0F,
    floor_half_size, floor_y, -floor_half_size,
    floor_uv_tiles, 0.0F,   0.0F, 1.0F, 0.0F,   1.0F, 0.0F, 0.0F,
};

constexpr auto floor_indices = std::array<uint32_t, 6>{0, 1, 2, 0, 2, 3};

// A small cluster of cubes hovering above the floor, in view, purely to
// give the reflective floor something visible to reflect.
constexpr auto cluster_size = 5;
constexpr auto cluster_spacing = 2.5F;
constexpr auto cluster_center_y = 4.0F;
constexpr auto cluster_center_z = 15.0F;

constexpr auto warmup_frames = 30;
constexpr auto measured_frames = 120;

constexpr auto max_average_frame_time_ms = 7.0;

auto make_cluster_model_matrices() -> std::vector<Maths::Matrix4x4f> {
    auto model_matrices = std::vector<Maths::Matrix4x4f>{};
    model_matrices.reserve(
        static_cast<size_t>(cluster_size) * static_cast<size_t>(cluster_size) *
        static_cast<size_t>(cluster_size)
    );

    constexpr auto offset =
        cluster_spacing * static_cast<float>(cluster_size - 1) / 2.0F;

    for (auto grid_x = 0; grid_x < cluster_size; ++grid_x) {
        for (auto grid_y = 0; grid_y < cluster_size; ++grid_y) {
            for (auto grid_z = 0; grid_z < cluster_size; ++grid_z) {
                const auto position = Maths::Vector3f{
                    (static_cast<float>(grid_x) * cluster_spacing) - offset,
                    cluster_center_y +
                        (static_cast<float>(grid_y) * cluster_spacing) - offset,
                    cluster_center_z +
                        (static_cast<float>(grid_z) * cluster_spacing) - offset,
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

    // Elevated and tilted down toward the floor, matching
    // Demo/TexturedModel's framing, so the reflective floor fills most of
    // the lower half of the viewport.
    constexpr auto camera_initial_position = Maths::Vector3f{0.0F, 8.0F, -20.0F};
    const auto camera_initial_forward =
        Maths::Vector3f{0.0F, -0.25F, 1.0F}.normalized();
    constexpr auto camera_far_plane = 300.0F;

    auto luminol_engine = RenderEngine(Properties{
        .title = "Luminol Screen Space Reflection Stress Test",
    });

    auto camera = Camera{CameraProperties{
        .position = camera_initial_position,
        .forward = camera_initial_forward,
        .far_plane = camera_far_plane,
    }};

    const auto floor_id = luminol_engine.get_renderer().create_renderable(
        floor_vertices,
        floor_indices,
        TexturePaths{
            .diffuse_texture_path = "res/textures/floor_albedo.png",
            // One combined map serves both slots: pbr_frag reads metallic
            // from the blue channel and roughness from the green channel.
            .metallic_texture_path =
                "res/textures/floor_metallic_roughness.png",
            .roughness_texture_path =
                "res/textures/floor_metallic_roughness.png",
        }
    );

    const auto cube_id =
        luminol_engine.get_renderer().create_renderable("res/models/cube/cube.obj");
    const auto cluster_matrices = make_cluster_model_matrices();

    constexpr auto floor_model_matrix = Maths::Matrix4x4f::identity();
    luminol_engine.get_renderer().queue_draw_instanced_static(
        floor_id, gsl::span{&floor_model_matrix, 1}
    );
    luminol_engine.get_renderer().queue_draw_instanced_static(
        cube_id, cluster_matrices
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
        "ScreenSpaceReflection stress test: %d cluster instances, %d "
        "frames measured (after %d warmup) - average %.3f ms/frame, worst "
        "%.3f ms/frame\n",
        static_cast<int>(cluster_matrices.size()),
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
