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

}  // namespace

auto main() -> int {
    using namespace Luminol;

    constexpr auto camera_initial_position = Maths::Vector3f{0.0f, 0.0f, -3.0f};
    constexpr auto camera_initial_forward = Maths::Vector3f{0.0f, 0.0f, 1.0f};
    constexpr auto camera_rotation_speed = 0.1f;
    constexpr auto camera_translation_speed = 2.0f;

    auto luminol_engine = Luminol::RenderEngine(Luminol::Properties{
        .graphics_api = Graphics::GraphicsApi::SDL_GPU,
    });

    auto camera = Graphics::Camera{CameraProperties{
        .position = camera_initial_position,
        .forward = camera_initial_forward,
        .translation_speed = camera_translation_speed,
        .rotation_speed = camera_rotation_speed,
    }};

    auto timer = Utilities::Timer{};

    const auto model_id =
        luminol_engine.get_renderer().create_renderable(
            "res/models/cut_fish/scene.gltf"
        );

    auto last_frame_time_seconds = 0.0;

    while (!luminol_engine.get_window().should_close()) {
        const auto current_frame_time_seconds = timer.elapsed_seconds();
        const auto delta_time_seconds =
            current_frame_time_seconds - last_frame_time_seconds;
        last_frame_time_seconds = current_frame_time_seconds;

        luminol_engine.get_window().poll_events();

        handle_key_events(
            luminol_engine, camera, gsl::narrow_cast<float>(delta_time_seconds)
        );

        const auto mouse_delta = luminol_engine.get_window().get_mouse_delta();
        camera.rotate(
            gsl::narrow_cast<float>(mouse_delta.delta_x),
            gsl::narrow_cast<float>(mouse_delta.delta_y)
        );

        constexpr auto color = Maths::Vector4f{0.0F, 0.0F, 0.0F, 1.0F};

        luminol_engine.get_renderer().clear_color(color);
        luminol_engine.get_renderer().clear(Graphics::BufferBit::ColorDepth);

        camera.set_aspect_ratio(
            static_cast<float>(luminol_engine.get_window().get_width()) /
            static_cast<float>(luminol_engine.get_window().get_height())
        );

        luminol_engine.get_renderer().set_view_matrix(camera.get_view_matrix());
        luminol_engine.get_renderer().set_projection_matrix(
            camera.get_projection_matrix()
        );

        luminol_engine.get_renderer().queue_draw(
            model_id, Maths::Matrix4x4f::identity()
        );

        luminol_engine.get_renderer().draw();

        luminol_engine.get_window().swap_buffers();
    }

    return 0;
}
