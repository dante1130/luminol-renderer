#include <array>
#include <cstdint>

#include <LuminolRenderEngine/LuminolRenderEngine.hpp>
#include <LuminolRenderEngine/Graphics/Camera.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPURenderer.hpp>
#include <LuminolRenderEngine/Graphics/TexturePaths.hpp>
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

    auto luminol_engine = Luminol::RenderEngine(Luminol::Properties{});

    auto camera = Graphics::Camera{CameraProperties{
        .position = camera_initial_position,
        .forward = camera_initial_forward,
        .translation_speed = camera_translation_speed,
        .rotation_speed = camera_rotation_speed,
    }};

    auto timer = Utilities::Timer{};

    // position.xyz          uv.xy       normal.xyz      tangent.xyz
    constexpr auto cube_vertices = std::array<float, size_t{11} * 24>{
        // Front (+Z)
        -0.5F,  0.5F,  0.5F,   0.0F, 0.0F,   0.0F, 0.0F, 1.0F,   1.0F, 0.0F, 0.0F,
        -0.5F, -0.5F,  0.5F,   0.0F, 1.0F,   0.0F, 0.0F, 1.0F,   1.0F, 0.0F, 0.0F,
         0.5F, -0.5F,  0.5F,   1.0F, 1.0F,   0.0F, 0.0F, 1.0F,   1.0F, 0.0F, 0.0F,
         0.5F,  0.5F,  0.5F,   1.0F, 0.0F,   0.0F, 0.0F, 1.0F,   1.0F, 0.0F, 0.0F,
        // Back (-Z)
         0.5F,  0.5F, -0.5F,   0.0F, 0.0F,   0.0F, 0.0F, -1.0F,  -1.0F, 0.0F, 0.0F,
         0.5F, -0.5F, -0.5F,   0.0F, 1.0F,   0.0F, 0.0F, -1.0F,  -1.0F, 0.0F, 0.0F,
        -0.5F, -0.5F, -0.5F,   1.0F, 1.0F,   0.0F, 0.0F, -1.0F,  -1.0F, 0.0F, 0.0F,
        -0.5F,  0.5F, -0.5F,   1.0F, 0.0F,   0.0F, 0.0F, -1.0F,  -1.0F, 0.0F, 0.0F,
        // Left (-X)
        -0.5F,  0.5F, -0.5F,   0.0F, 0.0F,   -1.0F, 0.0F, 0.0F,  0.0F, 0.0F, 1.0F,
        -0.5F, -0.5F, -0.5F,   0.0F, 1.0F,   -1.0F, 0.0F, 0.0F,  0.0F, 0.0F, 1.0F,
        -0.5F, -0.5F,  0.5F,   1.0F, 1.0F,   -1.0F, 0.0F, 0.0F,  0.0F, 0.0F, 1.0F,
        -0.5F,  0.5F,  0.5F,   1.0F, 0.0F,   -1.0F, 0.0F, 0.0F,  0.0F, 0.0F, 1.0F,
        // Right (+X)
         0.5F,  0.5F,  0.5F,   0.0F, 0.0F,   1.0F, 0.0F, 0.0F,   0.0F, 0.0F, -1.0F,
         0.5F, -0.5F,  0.5F,   0.0F, 1.0F,   1.0F, 0.0F, 0.0F,   0.0F, 0.0F, -1.0F,
         0.5F, -0.5F, -0.5F,   1.0F, 1.0F,   1.0F, 0.0F, 0.0F,   0.0F, 0.0F, -1.0F,
         0.5F,  0.5F, -0.5F,   1.0F, 0.0F,   1.0F, 0.0F, 0.0F,   0.0F, 0.0F, -1.0F,
        // Top (+Y)
        -0.5F,  0.5F, -0.5F,   0.0F, 0.0F,   0.0F, 1.0F, 0.0F,   1.0F, 0.0F, 0.0F,
        -0.5F,  0.5F,  0.5F,   0.0F, 1.0F,   0.0F, 1.0F, 0.0F,   1.0F, 0.0F, 0.0F,
         0.5F,  0.5F,  0.5F,   1.0F, 1.0F,   0.0F, 1.0F, 0.0F,   1.0F, 0.0F, 0.0F,
         0.5F,  0.5F, -0.5F,   1.0F, 0.0F,   0.0F, 1.0F, 0.0F,   1.0F, 0.0F, 0.0F,
        // Bottom (-Y)
        -0.5F, -0.5F,  0.5F,   0.0F, 0.0F,   0.0F, -1.0F, 0.0F,  1.0F, 0.0F, 0.0F,
        -0.5F, -0.5F, -0.5F,   0.0F, 1.0F,   0.0F, -1.0F, 0.0F,  1.0F, 0.0F, 0.0F,
         0.5F, -0.5F, -0.5F,   1.0F, 1.0F,   0.0F, -1.0F, 0.0F,  1.0F, 0.0F, 0.0F,
         0.5F, -0.5F,  0.5F,   1.0F, 0.0F,   0.0F, -1.0F, 0.0F,  1.0F, 0.0F, 0.0F,
    };

    constexpr auto cube_indices = std::array<uint32_t, 36>{
        0,  1,  2,  0,  2,  3,   // Front
        4,  5,  6,  4,  6,  7,   // Back
        8,  9,  10, 8,  10, 11,  // Left
        12, 13, 14, 12, 14, 15,  // Right
        16, 17, 18, 16, 18, 19,  // Top
        20, 21, 22, 20, 22, 23,  // Bottom
    };

    const auto cube_id =
        luminol_engine.get_renderer().create_renderable(
            cube_vertices,
            cube_indices,
            Graphics::TexturePaths{
                .diffuse_texture_path = "res/textures/reflex.png"
            }
        );

    const auto cube_model_matrix = Maths::Matrix4x4f::identity();
    luminol_engine.get_renderer().queue_draw_instanced_static(
        cube_id, gsl::span{&cube_model_matrix, 1}
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

        camera.set_aspect_ratio(
            static_cast<float>(luminol_engine.get_window().get_width()) /
            static_cast<float>(luminol_engine.get_window().get_height())
        );

        luminol_engine.get_renderer().set_view_matrix(camera.get_view_matrix());
        luminol_engine.get_renderer().set_projection_matrix(
            camera.get_projection_matrix()
        );

        luminol_engine.get_renderer().draw();
    }

    return 0;
}
