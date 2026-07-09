#include <LuminolRenderEngine/LuminolRenderEngine.hpp>

auto main() -> int {
    using namespace Luminol;

    auto luminol_engine = Luminol::RenderEngine(Luminol::Properties{
        .graphics_api = Graphics::GraphicsApi::SDL_GPU,
    });

    const auto font_id = luminol_engine.get_renderer().create_font(
        "res/fonts/RobotoMono-Regular.ttf", 24.0F
    );

    while (!luminol_engine.get_window().should_close()) {
        luminol_engine.get_window().poll_events();

        constexpr auto color = Maths::Vector4f{0.0F, 0.0F, 0.0F, 1.0F};

        luminol_engine.get_renderer().clear_color(color);

        luminol_engine.get_renderer().queue_draw_text(
            font_id,
            "Hello, Luminol!",
            Maths::Vector2f{10.0F, 10.0F},
            Maths::Vector4f{1.0F, 1.0F, 1.0F, 1.0F}
        );

        luminol_engine.get_renderer().draw();
    }

    return 0;
}
