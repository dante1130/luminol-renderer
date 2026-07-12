#include <cstdio>
#include <vector>

#include <LuminolMaths/Transform.hpp>
#include <LuminolRenderEngine/LuminolRenderEngine.hpp>
#include <LuminolRenderEngine/Graphics/Camera.hpp>
#include <LuminolRenderEngine/Utilities/Timer.hpp>

namespace {

using namespace Luminol;
using namespace Luminol::Graphics;

auto handle_key_events(
    RenderEngine& engine, Camera& camera, float delta_time_seconds
) -> void {
    if (engine.get_window().is_key_event('w', KeyEvent::Press)) {
        camera.move(CameraMovement::Forward, delta_time_seconds);
    }

    if (engine.get_window().is_key_event('s', KeyEvent::Press)) {
        camera.move(CameraMovement::Backward, delta_time_seconds);
    }

    if (engine.get_window().is_key_event('a', KeyEvent::Press)) {
        camera.move(CameraMovement::Left, delta_time_seconds);
    }

    if (engine.get_window().is_key_event('d', KeyEvent::Press)) {
        camera.move(CameraMovement::Right, delta_time_seconds);
    }

    if (engine.get_window().is_key_event('q', KeyEvent::Press)) {
        engine.get_window().close();
    }
}

// Debug-only: 'o' toggles the occlusion test on/off (frustum culling still
// applies) for A/B comparison, 'p' logs the current visible instance count.
// is_key_event is level-triggered (true every frame the key is held), so
// both need press-edge tracking to fire once per physical key press.
auto handle_debug_occlusion_keys(
    RenderEngine& engine, bool& occlusion_disabled, bool& o_key_was_down,
    bool& p_key_was_down, bool& hiz_visible, bool& v_key_was_down
) -> void {
    const auto o_key_down = engine.get_window().is_key_event('o', KeyEvent::Press);
    if (o_key_down && !o_key_was_down) {
        occlusion_disabled = !occlusion_disabled;
        engine.get_renderer().set_debug_disable_occlusion_culling(
            occlusion_disabled
        );
        std::printf(
            "[OcclusionDebug] occlusion culling %s\n",
            occlusion_disabled ? "DISABLED" : "enabled"
        );
    }
    o_key_was_down = o_key_down;

    const auto p_key_down = engine.get_window().is_key_event('p', KeyEvent::Press);
    if (p_key_down && !p_key_was_down) {
        engine.get_renderer().debug_log_visible_instance_count();
    }
    p_key_was_down = p_key_down;

    const auto v_key_down = engine.get_window().is_key_event('v', KeyEvent::Press);
    if (v_key_down && !v_key_was_down) {
        hiz_visible = !hiz_visible;
        engine.get_renderer().set_debug_visualize_hiz(hiz_visible);
        std::printf(
            "[OcclusionDebug] Hi-Z pyramid visualization %s\n",
            hiz_visible ? "ON" : "off"
        );
    }
    v_key_was_down = v_key_down;
}

constexpr auto grid_size = 100;
constexpr auto grid_spacing = 5.0F;

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

    constexpr auto camera_initial_position =
        Maths::Vector3f{0.0f, 0.0f, -150.0f};
    constexpr auto camera_initial_forward = Maths::Vector3f{0.0f, 0.0f, 1.0f};
    constexpr auto camera_rotation_speed = 0.1f;
    constexpr auto camera_translation_speed = 40.0f;
    constexpr auto camera_far_plane = 500.0f;

    auto luminol_engine = Luminol::RenderEngine(Luminol::Properties{});

    auto camera = Graphics::Camera{CameraProperties{
        .position = camera_initial_position,
        .forward = camera_initial_forward,
        .far_plane = camera_far_plane,
        .translation_speed = camera_translation_speed,
        .rotation_speed = camera_rotation_speed,
    }};

    auto timer = Utilities::Timer{};

    const auto model_id =
        luminol_engine.get_renderer().create_renderable(
            "res/models/cube/cube.obj"
        );

    const auto model_matrices = make_grid_model_matrices();

    auto last_frame_time_seconds = 0.0;

    auto occlusion_disabled = false;
    auto o_key_was_down = false;
    auto p_key_was_down = false;
    auto hiz_visible = false;
    auto v_key_was_down = false;

    while (!luminol_engine.get_window().should_close()) {
        const auto current_frame_time_seconds = timer.elapsed_seconds();
        const auto delta_time_seconds =
            current_frame_time_seconds - last_frame_time_seconds;
        last_frame_time_seconds = current_frame_time_seconds;

        luminol_engine.get_window().poll_events();

        handle_key_events(
            luminol_engine, camera, gsl::narrow_cast<float>(delta_time_seconds)
        );

        handle_debug_occlusion_keys(
            luminol_engine, occlusion_disabled, o_key_was_down, p_key_was_down,
            hiz_visible, v_key_was_down
        );

        const auto mouse_delta = luminol_engine.get_window().get_mouse_delta();
        camera.rotate(
            gsl::narrow_cast<float>(mouse_delta.delta_x),
            gsl::narrow_cast<float>(mouse_delta.delta_y)
        );

        constexpr auto color = Maths::Vector4f{0.0F, 0.0F, 0.0F, 1.0F};

        luminol_engine.get_renderer().clear_color(color);

        camera.set_aspect_ratio(
            static_cast<float>(luminol_engine.get_window().get_width()) /
            static_cast<float>(luminol_engine.get_window().get_height())
        );

        luminol_engine.get_renderer().set_view_matrix(camera.get_view_matrix());
        luminol_engine.get_renderer().set_projection_matrix(
            camera.get_projection_matrix()
        );

        luminol_engine.get_renderer().queue_draw_instanced(
            model_id, model_matrices
        );

        luminol_engine.get_renderer().draw();
    }

    return 0;
}
