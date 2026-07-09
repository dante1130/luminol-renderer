#include <array>
#include <cstdint>

#include <LuminolRenderEngine/LuminolRenderEngine.hpp>
#include <LuminolRenderEngine/Graphics/TexturePaths.hpp>

auto main() -> int {
    using namespace Luminol;

    auto luminol_engine = Luminol::RenderEngine(Luminol::Properties{
        .graphics_api = Graphics::GraphicsApi::SDL_GPU,
    });

    // position.xyz          uv.xy       normal.xyz      tangent.xyz
    constexpr auto quad_vertices = std::array<float, size_t{11} * 4>{
        -0.5F,  0.5F, 0.0F,   0.0F, 0.0F,   0.0F, 0.0F, 1.0F,   1.0F, 0.0F, 0.0F,
        -0.5F, -0.5F, 0.0F,   0.0F, 1.0F,   0.0F, 0.0F, 1.0F,   1.0F, 0.0F, 0.0F,
         0.5F, -0.5F, 0.0F,   1.0F, 1.0F,   0.0F, 0.0F, 1.0F,   1.0F, 0.0F, 0.0F,
         0.5F,  0.5F, 0.0F,   1.0F, 0.0F,   0.0F, 0.0F, 1.0F,   1.0F, 0.0F, 0.0F,
    };
    constexpr auto quad_indices = std::array<uint32_t, 6>{0, 2, 1, 0, 3, 2};

    const auto quad_id =
        luminol_engine.get_renderer().create_renderable(
            quad_vertices,
            quad_indices,
            Graphics::TexturePaths{
                .diffuse_texture_path = "res/textures/reflex.png"
            }
        );

    while (!luminol_engine.get_window().should_close()) {
        luminol_engine.get_window().poll_events();

        constexpr auto color = Maths::Vector4f{0.0F, 0.0F, 0.0F, 1.0F};

        luminol_engine.get_renderer().clear_color(color);

        luminol_engine.get_renderer().queue_draw(
            quad_id, Maths::Matrix4x4f::identity()
        );

        luminol_engine.get_renderer().draw();
    }

    return 0;
}
