#include <LuminolMaths/Transform.hpp>

#include <LuminolRenderEngine/LuminolRenderEngine.hpp>
#include <LuminolRenderEngine/Graphics/Camera.hpp>
#include <LuminolRenderEngine/Utilities/Timer.hpp>

auto main() -> int {
    using namespace Luminol;

    auto luminol_engine = Luminol::RenderEngine(Luminol::Properties{});

    while (!luminol_engine.get_window().should_close()) {
        luminol_engine.get_window().poll_events();

        constexpr auto color = Maths::Vector4f{0.0f, 0.0f, 0.0f, 1.0f};

        luminol_engine.get_renderer().clear_color(color);
        luminol_engine.get_renderer().clear(Graphics::BufferBit::ColorDepth);

        luminol_engine.get_renderer().draw();

        luminol_engine.get_window().swap_buffers();
    }

    return 0;
}
