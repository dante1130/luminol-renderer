#include <cstdint>

#include <LuminolMaths/Transform.hpp>

#include <LuminolRenderEngine/LuminolRenderEngine.hpp>
#include <LuminolRenderEngine/Graphics/Camera.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPURenderer.hpp>
#include <LuminolRenderEngine/Utilities/ImageLoader.hpp>
#include <LuminolRenderEngine/Utilities/Timer.hpp>

#include "SphereMesh.hpp"

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

// Builds a solid-color 1x1 RGBA8 in-memory image - no file I/O, no PNG
// encoding. create_texture_from_image only reads width/height/data, so a
// hand-built 1x1 buffer is valid input (see SDL_GPUMesh.cpp).
auto make_flat_color_image(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha)
    -> Utilities::ImageLoader::Image {
    return Utilities::ImageLoader::Image{
        .path = {},
        .data = {red, green, blue, alpha},
        .width = 1,
        .height = 1,
        .channels = 4,
    };
}

constexpr auto grid_dimension = 7U;
constexpr auto sphere_radius = 0.9F;
constexpr auto sphere_spacing = 2.2F;
constexpr auto grid_extent =
    sphere_spacing * static_cast<float>(grid_dimension - 1U);
constexpr auto grid_offset = grid_extent / 2.0F;

// Shared base albedo color across every sphere - only metallic/roughness
// vary across the grid.
constexpr auto base_albedo_red = uint8_t{204};
constexpr auto base_albedo_green = uint8_t{26};
constexpr auto base_albedo_blue = uint8_t{26};

constexpr auto point_light_brightness = 30.0F;

}  // namespace

auto main() -> int {
    using namespace Luminol;

    constexpr auto camera_initial_position =
        Maths::Vector3f{0.0F, 0.0F, -((grid_extent * 1.4F) + 6.0F)};
    constexpr auto camera_initial_forward = Maths::Vector3f{0.0F, 0.0F, 1.0F};
    constexpr auto camera_rotation_speed = 0.1F;
    constexpr auto camera_translation_speed = 5.0F;

    auto luminol_engine = Luminol::RenderEngine(Luminol::Properties{});

    auto camera = Graphics::Camera{CameraProperties{
        .position = camera_initial_position,
        .forward = camera_initial_forward,
        .translation_speed = camera_translation_speed,
        .rotation_speed = camera_rotation_speed,
    }};

    auto timer = Utilities::Timer{};

    const auto sphere_mesh =
        Demo::PbrSpheres::make_uv_sphere(sphere_radius, 24U, 32U);

    const auto diffuse_image = make_flat_color_image(
        base_albedo_red, base_albedo_green, base_albedo_blue, 255
    );

    for (auto row = 0U; row < grid_dimension; ++row) {
        const auto metallic =
            static_cast<float>(row) / static_cast<float>(grid_dimension - 1U);

        for (auto column = 0U; column < grid_dimension; ++column) {
            const auto roughness = static_cast<float>(column) /
                                    static_cast<float>(grid_dimension - 1U);

            const auto orm_image = make_flat_color_image(
                255,
                static_cast<uint8_t>(roughness * 255.0F),
                static_cast<uint8_t>(metallic * 255.0F),
                255
            );

            const auto renderable_id = luminol_engine.get_renderer().create_renderable(
                sphere_mesh.vertices,
                sphere_mesh.indices,
                Graphics::SDL_GPU::TextureImages{
                    .diffuse_texture = diffuse_image,
                    .metallic_texture = orm_image,
                    .roughness_shares_metallic_source = true,
                    .ambient_occlusion_shares_metallic_source = true,
                }
            );

            const auto position = Maths::Vector3f{
                (static_cast<float>(column) * sphere_spacing) - grid_offset,
                (static_cast<float>(row) * sphere_spacing) - grid_offset,
                0.0F,
            };
            const auto sphere_model_matrix =
                Maths::Transform::translate_4x4(position);

            luminol_engine.get_renderer().queue_draw_instanced_static(
                renderable_id, gsl::span{&sphere_model_matrix, 1}
            );
        }
    }

    constexpr auto directional_light = Graphics::DirectionalLight{
        .direction = Maths::Vector3f{0.3F, -0.5F, 1.0F},
        .color = Maths::Vector3f{1.0F, 1.0F, 1.0F},
    };
    luminol_engine.get_renderer().get_light_manager().update_directional_light(
        directional_light
    );

    // Two point lights off to either side, bright enough to show off
    // specular highlights across the metallic/roughness gradient distinct
    // from the directional light's contribution.
    const auto left_point_light = Graphics::PointLight{
        .position = Maths::Vector3f{-grid_extent, grid_extent * 0.5F, -grid_extent * 0.8F},
        .color = Maths::Vector3f{
            point_light_brightness, point_light_brightness, point_light_brightness
        },
    };
    const auto right_point_light = Graphics::PointLight{
        .position = Maths::Vector3f{grid_extent, grid_extent * 0.5F, -grid_extent * 0.8F},
        .color = Maths::Vector3f{
            point_light_brightness, point_light_brightness, point_light_brightness
        },
    };
    [[maybe_unused]] const auto left_point_light_id =
        luminol_engine.get_renderer().get_light_manager().add_point_light(
            left_point_light
        );
    [[maybe_unused]] const auto right_point_light_id =
        luminol_engine.get_renderer().get_light_manager().add_point_light(
            right_point_light
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
