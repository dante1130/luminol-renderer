#include <array>
#include <cstdint>

#include <LuminolMaths/Transform.hpp>

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

    // Elevated and tilted down toward the floor so the screen-space
    // reflections in the reflective floor below are visible immediately.
    constexpr auto camera_initial_position = Maths::Vector3f{0.0f, 1.0f, -4.0f};
    constexpr auto camera_initial_forward = Maths::Vector3f{0.0f, -0.25f, 1.0f};
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

    const auto model_id =
        luminol_engine.get_renderer().create_renderable(
            "res/models/cut_fish/scene.gltf"
        );

    // A large flat +Y-facing quad to validate screen-space reflections. Kept
    // low-roughness and metallic (see the texture paths below) so the scene
    // above it reflects sharply. Vertex layout: position.xyz, uv.xy,
    // normal.xyz, tangent.xyz (11 floats), matching the other procedural
    // demos.
    constexpr auto floor_half_size = 20.0f;
    constexpr auto floor_uv_tiles = 10.0f;
    constexpr auto floor_vertices = std::array<float, size_t{11} * 4>{
        -floor_half_size, 0.0f, -floor_half_size,
        0.0f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f, 0.0f,
        -floor_half_size, 0.0f,  floor_half_size,
        0.0f, floor_uv_tiles,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f, 0.0f,
         floor_half_size, 0.0f,  floor_half_size,
        floor_uv_tiles, floor_uv_tiles,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f, 0.0f,
         floor_half_size, 0.0f, -floor_half_size,
        floor_uv_tiles, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f, 0.0f,
    };

    constexpr auto floor_indices = std::array<uint32_t, 6>{0, 1, 2, 0, 2, 3};

    const auto floor_id = luminol_engine.get_renderer().create_renderable(
        floor_vertices,
        floor_indices,
        Graphics::TexturePaths{
            .diffuse_texture_path = "res/textures/floor_albedo.png",
            // One combined map serves both slots: pbr_frag reads metallic from
            // the blue channel and roughness from the green channel.
            .metallic_texture_path =
                "res/textures/floor_metallic_roughness.png",
            .roughness_texture_path =
                "res/textures/floor_metallic_roughness.png",
        }
    );

    // Sit the floor one unit below the model so the model reflects in it.
    const auto floor_model_matrix =
        Maths::Transform::translate_4x4(Maths::Vector3f{0.0f, -1.0f, 0.0f});

    constexpr auto directional_light = Graphics::DirectionalLight{
        .direction = Maths::Vector3f{0.5f, -0.5f, 1.0f},
        .color = Maths::Vector3f{1.0f, 1.0f, 1.0f},
    };

    luminol_engine.get_renderer().get_light_manager().update_directional_light(
        directional_light
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

        luminol_engine.get_renderer().queue_draw(
            model_id, Maths::Matrix4x4f::identity()
        );

        luminol_engine.get_renderer().queue_draw(floor_id, floor_model_matrix);

        luminol_engine.get_renderer().draw();
    }

    return 0;
}
