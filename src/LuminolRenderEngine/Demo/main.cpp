#include <iostream>
#include <random>

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <LuminolRenderEngine/LuminolRenderEngine.hpp>
#include <LuminolRenderEngine/Graphics/Camera.hpp>
#include <LuminolRenderEngine/Utilities/Timer.hpp>

namespace {

using namespace Luminol;
using namespace Luminol::Graphics;

struct LightDrawData {
    glm::mat4 model_matrix;
    glm::vec3 color;
    LightManager::LightId light_id;
};

struct Lights {
    RenderableId renderable_id;
    std::vector<LightDrawData> light_data;
};

auto handle_key_events(
    RenderEngine& engine, Camera& camera, float delta_time_seconds
) -> void {
    if (engine.get_window().is_key_event('W', KeyEvent::Press)) {
        camera.move(Graphics::CameraMovement::Forward, delta_time_seconds);
    }

    if (engine.get_window().is_key_event('S', KeyEvent::Press)) {
        camera.move(Graphics::CameraMovement::Backward, delta_time_seconds);
    }

    if (engine.get_window().is_key_event('A', KeyEvent::Press)) {
        camera.move(Graphics::CameraMovement::Left, delta_time_seconds);
    }

    if (engine.get_window().is_key_event('D', KeyEvent::Press)) {
        camera.move(Graphics::CameraMovement::Right, delta_time_seconds);
    }

    if (engine.get_window().is_key_event('Q', KeyEvent::Press)) {
        engine.get_window().close();
    }
}

}  // namespace

auto main() -> int {
    using namespace Luminol;

    constexpr auto camera_initial_position = glm::vec3(5.0f, 0.0f, 0.0f);
    constexpr auto camera_initial_forward = glm::vec3(-1.0f, 0.0f, 0.0f);
    constexpr auto camera_rotation_speed = 0.1f;

    auto luminol_engine = Luminol::RenderEngine(Luminol::Properties{});
    auto camera = Graphics::Camera{CameraProperties{
        .position = camera_initial_position,
        .forward = camera_initial_forward,
        .rotation_speed = camera_rotation_speed,
    }};
    auto timer = Utilities::Timer{};

    constexpr auto exposure = 2.0f;
    luminol_engine.get_renderer().set_exposure(exposure);

    auto model_id =
        luminol_engine.get_renderer()
            .get_renderable_manager()
            .create_renderable("res/models/Sponza/glTF/Sponza.gltf");

    constexpr auto directional_light = Graphics::DirectionalLight{
        .direction = glm::vec3(0.5f, -0.5f, 1.0f),
        .color = glm::vec3(0.1f, 0.1f, 0.1f),
    };

    luminol_engine.get_renderer().get_light_manager().update_directional_light(
        directional_light
    );

    constexpr auto lights_count = 8u;

    auto lights = Lights{
        .renderable_id = luminol_engine.get_renderer()
                             .get_renderable_manager()
                             .create_renderable("res/models/cube/cube.obj")
    };

    lights.light_data.reserve(lights_count);

    auto random = std::mt19937{std::random_device{}()};

    for (auto i = 0u; i < lights_count; ++i) {
        const auto position = glm::vec3(
            std::uniform_real_distribution<float>(-5.0f, 5.0f)(random),
            std::uniform_real_distribution<float>(-5.0f, 5.0f)(random) + 5.0f,
            std::uniform_real_distribution<float>(-5.0f, 5.0f)(random)
        );

        const auto color = glm::vec3(
            std::uniform_real_distribution<float>(0.0f, 1.0f)(random),
            std::uniform_real_distribution<float>(0.0f, 1.0f)(random),
            std::uniform_real_distribution<float>(0.0f, 1.0f)(random)
        );

        constexpr auto scale = glm::vec3(0.1f, 0.1f, 0.1f);

        auto model_matrix = glm::mat4(1.0f);
        model_matrix = glm::translate(model_matrix, position);
        model_matrix = glm::scale(model_matrix, scale);

        constexpr auto intensity = 1.0f;

        const auto point_light_id_opt =
            luminol_engine.get_renderer().get_light_manager().add_point_light(
                PointLight{
                    .position = position,
                    .color = color * intensity,
                }
            );

        if (!point_light_id_opt.has_value()) {
            std::cerr << "Failed to add point light\n";
            return 1;
        }

        lights.light_data.emplace_back(LightDrawData{
            .model_matrix = model_matrix,
            .color = color,
            .light_id = point_light_id_opt.value()
        });
    }

    const auto initial_flash_light = Graphics::SpotLight{
        .position = camera.get_position(),
        .direction = camera.get_forward(),
        .color = glm::vec3(1.0f, 1.0f, 1.0f),
        .cut_off = glm::cos(glm::radians(12.5f)),
        .outer_cut_off = glm::cos(glm::radians(17.5f))
    };

    auto flash_light = initial_flash_light;

    const auto flash_light_id_opt =
        luminol_engine.get_renderer().get_light_manager().add_spot_light(
            flash_light
        );

    if (!flash_light_id_opt.has_value()) {
        std::cerr << "Failed to add flash light\n";
        return 1;
    }

    const auto flash_light_id = flash_light_id_opt.value();

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

        constexpr auto color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

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

        flash_light.position = camera.get_position();
        flash_light.direction = camera.get_forward();

        luminol_engine.get_renderer().get_light_manager().update_spot_light(
            flash_light_id, flash_light
        );

        for (auto& light_data : lights.light_data) {
            constexpr auto rotation_degrees = 90.0f;

            auto rotation = glm::rotate(
                glm::mat4(1.0f),
                glm::radians(rotation_degrees) *
                    gsl::narrow_cast<float>(delta_time_seconds),
                glm::vec3(0.0f, 1.0f, 0.0f)
            );

            light_data.model_matrix = rotation * light_data.model_matrix;

            luminol_engine.get_renderer()
                .get_light_manager()
                .update_point_light(
                    light_data.light_id,
                    PointLight{
                        .position = light_data.model_matrix[3],
                        .color = light_data.color,
                    }
                );

            luminol_engine.get_renderer().queue_draw_with_color(
                lights.renderable_id, light_data.model_matrix, light_data.color
            );
        }

        {
            constexpr auto scale = glm::vec3(1.0f);

            auto model_matrix = glm::mat4(1.0f);
            model_matrix = glm::scale(model_matrix, scale);

            luminol_engine.get_renderer().queue_draw(model_id, model_matrix);
        }

        luminol_engine.get_renderer().draw();

        luminol_engine.get_window().swap_buffers();
    }

    return 0;
}
