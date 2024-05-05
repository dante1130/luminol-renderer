#include <iostream>
#include <random>

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <LuminolRenderEngine/LuminolRenderEngine.hpp>
#include <LuminolRenderEngine/Utilities/Timer.hpp>

namespace {

using namespace Luminol;
using namespace Luminol::Graphics;

struct LightEntity {
    glm::vec3 position;
    glm::vec3 color;
    std::unique_ptr<Model> model;
};

auto handle_key_events(RenderEngine& engine, float delta_time_seconds) -> void {
    if (engine.get_window().is_key_event('W', KeyEvent::Press)) {
        engine.get_camera().move(
            Graphics::CameraMovement::Forward, delta_time_seconds
        );
    }

    if (engine.get_window().is_key_event('S', KeyEvent::Press)) {
        engine.get_camera().move(
            Graphics::CameraMovement::Backward, delta_time_seconds
        );
    }

    if (engine.get_window().is_key_event('A', KeyEvent::Press)) {
        engine.get_camera().move(
            Graphics::CameraMovement::Left, delta_time_seconds
        );
    }

    if (engine.get_window().is_key_event('D', KeyEvent::Press)) {
        engine.get_camera().move(
            Graphics::CameraMovement::Right, delta_time_seconds
        );
    }

    if (engine.get_window().is_key_event('Q', KeyEvent::Press)) {
        engine.get_window().close();
    }
}

}  // namespace

auto main() -> int {
    using namespace Luminol;

    auto luminol_engine = Luminol::RenderEngine(Luminol::Properties{});
    auto timer = Utilities::Timer{};

    constexpr auto exposure = 2.0f;
    luminol_engine.get_renderer().set_exposure(exposure);

    auto model = luminol_engine.get_graphics_factory().create_model(
        "res/models/cut_fish/scene.gltf"
    );

    constexpr auto directional_light = Graphics::DirectionalLight{
        .direction = glm::vec3(0.5f, -0.5f, 1.0f),
        .color = glm::vec3(0.1f, 0.1f, 0.1f),
    };

    luminol_engine.get_renderer().get_light_manager().update_directional_light(
        directional_light
    );

    constexpr auto lights_count = 256u;

    auto entities = std::vector<LightEntity>{};
    entities.reserve(lights_count);

    auto random = std::mt19937{std::random_device{}()};

    for (auto i = 0u; i < lights_count; ++i) {
        const auto position = glm::vec3(
            std::uniform_real_distribution<float>(-5.0f, 5.0f)(random),
            std::uniform_real_distribution<float>(-5.0f, 5.0f)(random),
            std::uniform_real_distribution<float>(-5.0f, 5.0f)(random)
        );

        const auto color = glm::vec3(
            std::uniform_real_distribution<float>(0.0f, 1.0f)(random),
            std::uniform_real_distribution<float>(0.0f, 1.0f)(random),
            std::uniform_real_distribution<float>(0.0f, 1.0f)(random)
        );

        entities.emplace_back(
            position,
            color,
            luminol_engine.get_graphics_factory().create_model(
                "res/models/cube/cube.obj"
            )
        );

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
    }

    const auto initial_flash_light = Graphics::SpotLight{
        .position = luminol_engine.get_camera().get_position(),
        .direction = luminol_engine.get_camera().get_forward(),
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
            luminol_engine, gsl::narrow_cast<float>(delta_time_seconds)
        );

        const auto mouse_delta = luminol_engine.get_window().get_mouse_delta();
        luminol_engine.get_camera().rotate(
            gsl::narrow_cast<float>(mouse_delta.delta_x),
            gsl::narrow_cast<float>(mouse_delta.delta_y)
        );

        constexpr auto color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

        luminol_engine.get_renderer().clear_color(color);
        luminol_engine.get_renderer().clear(Graphics::BufferBit::ColorDepth);

        luminol_engine.get_camera().set_aspect_ratio(
            static_cast<float>(luminol_engine.get_window().get_width()) /
            static_cast<float>(luminol_engine.get_window().get_height())
        );

        luminol_engine.get_renderer().set_view_matrix(
            luminol_engine.get_camera().get_view_matrix()
        );
        luminol_engine.get_renderer().set_projection_matrix(
            luminol_engine.get_camera().get_projection_matrix()
        );

        flash_light.position = luminol_engine.get_camera().get_position();
        flash_light.direction = luminol_engine.get_camera().get_forward();

        luminol_engine.get_renderer().get_light_manager().update_spot_light(
            flash_light_id, flash_light
        );

        for (const auto& entity : entities) {
            constexpr auto scale = glm::vec3(0.1f, 0.1f, 0.1f);

            auto model_matrix = glm::mat4(1.0f);
            model_matrix = glm::translate(model_matrix, entity.position);
            model_matrix = glm::scale(model_matrix, scale);

            luminol_engine.get_renderer().queue_draw_with_color(
                *entity.model, model_matrix, entity.color
            );
        }

        {
            constexpr auto scale = glm::vec3(1.0f);

            auto model_matrix = glm::mat4(1.0f);
            model_matrix = glm::scale(model_matrix, scale);

            luminol_engine.get_renderer().queue_draw(*model, model_matrix);
        }

        luminol_engine.get_renderer().draw();

        luminol_engine.get_window().swap_buffers();
    }

    return 0;
}
