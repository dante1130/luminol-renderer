#include <array>
#include <cstdint>

#include <LuminolRenderEngine/LuminolRenderEngine.hpp>
#include <LuminolRenderEngine/Graphics/TexturePaths.hpp>

auto main() -> int {
    using namespace Luminol;

    auto luminol_engine = Luminol::RenderEngine(Luminol::Properties{
        .graphics_api = Graphics::GraphicsApi::SDL_GPU,
    });

    constexpr auto triangle_vertices = std::array<float, size_t{11} * 3>{
        // position.xyz          uv.xy       normal.xyz      tangent.xyz
        0.0F,  0.5F,  0.0F,   0.0F, 0.0F,   0.0F, 0.0F, 1.0F,   1.0F, 0.0F, 0.0F,
       -0.5F, -0.5F,  0.0F,   0.0F, 0.0F,   0.0F, 0.0F, 1.0F,   1.0F, 0.0F, 0.0F,
        0.5F, -0.5F,  0.0F,   0.0F, 0.0F,   0.0F, 0.0F, 1.0F,   1.0F, 0.0F, 0.0F,
    };
    constexpr auto triangle_indices = std::array<uint32_t, 3>{0, 1, 2};

    const auto triangle_id =
        luminol_engine.get_renderer().get_renderable_manager().create_renderable(
            triangle_vertices, triangle_indices, Graphics::TexturePaths{}
        );

    while (!luminol_engine.get_window().should_close()) {
        luminol_engine.get_window().poll_events();

        constexpr auto color = Maths::Vector4f{1.0F, 0.0F, 0.0F, 1.0F};

        luminol_engine.get_renderer().clear_color(color);
        luminol_engine.get_renderer().clear(Graphics::BufferBit::ColorDepth);

        luminol_engine.get_renderer().queue_draw(
            triangle_id, Maths::Matrix4x4f::identity()
        );

        luminol_engine.get_renderer().draw();

        luminol_engine.get_window().swap_buffers();
    }

    return 0;
}
