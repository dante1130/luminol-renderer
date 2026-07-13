#include <cstdio>
#include <string>

#include <LuminolMaths/Vector.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPURenderer.hpp>
#include <LuminolRenderEngine/LuminolRenderEngine.hpp>
#include <LuminolRenderEngine/Utilities/Timer.hpp>

// Headless stress test for SDL_GPUTextRenderPass. Unlike geometry, text is
// only drawn when queue_draw_text is called - so this is the one subsystem
// with genuinely zero incidental coverage from the other stress tests.
// SDL_GPUFont pre-bakes every printable-ASCII glyph into one atlas texture
// at font-load time (see SDL_GPUFont.cpp), so per-frame text rendering never
// rasterizes or allocates GPU resources regardless of how many strings
// change - this test embeds the current frame number in every label's text
// so every string differs every frame, exercising the worst case for
// SDL_GPUTextRenderPass::queue_draw's per-glyph quad generation and
// flush_frame_geometry's per-font vertex-buffer upload.
//
// THRESHOLD CALIBRATION: measured ~3.3ms/frame average (worst ~3.9ms) on an
// RTX 4080 Laptop after the glyph-atlas rewrite; max_average_frame_time_ms
// below is set to roughly 2.4x that baseline.

namespace {

using namespace Luminol;
using namespace Luminol::Graphics;

constexpr auto label_grid_columns = 20;
constexpr auto label_grid_rows = 20;
constexpr auto label_spacing_x = 60.0F;
constexpr auto label_spacing_y = 30.0F;
constexpr auto font_point_size = 16.0F;

constexpr auto warmup_frames = 30;
constexpr auto measured_frames = 120;

constexpr auto max_average_frame_time_ms = 3.4;

}  // namespace

auto main() -> int {
    using namespace Luminol;
    using namespace Luminol::Graphics;

    auto luminol_engine = RenderEngine(Properties{
        .title = "Luminol Text Rendering Stress Test",
    });

    const auto font_id = luminol_engine.get_renderer().create_font(
        "res/fonts/RobotoMono-Regular.ttf", font_point_size
    );

    constexpr auto color = Maths::Vector4f{0.0F, 0.0F, 0.0F, 1.0F};
    constexpr auto text_color = Maths::Vector4f{1.0F, 1.0F, 1.0F, 1.0F};
    constexpr auto label_count = label_grid_columns * label_grid_rows;

    auto run_frame = [&](int frame_number) {
        luminol_engine.get_renderer().clear_color(color);

        for (auto row = 0; row < label_grid_rows; ++row) {
            for (auto column = 0; column < label_grid_columns; ++column) {
                const auto position = Maths::Vector2f{
                    static_cast<float>(column) * label_spacing_x,
                    static_cast<float>(row) * label_spacing_y,
                };
                const auto text = std::to_string(frame_number) + ":" +
                    std::to_string(row) + "," + std::to_string(column);

                luminol_engine.get_renderer().queue_draw_text(
                    font_id, text, position, text_color
                );
            }
        }

        luminol_engine.get_renderer().draw();
    };

    for (auto frame = 0; frame < warmup_frames; ++frame) {
        run_frame(frame);
    }

    auto total_frame_time_seconds = 0.0;
    auto worst_frame_time_seconds = 0.0;

    for (auto frame = 0; frame < measured_frames; ++frame) {
        auto timer = Utilities::Timer{};
        run_frame(warmup_frames + frame);
        const auto frame_time_seconds = timer.elapsed_seconds();

        total_frame_time_seconds += frame_time_seconds;
        if (frame_time_seconds > worst_frame_time_seconds) {
            worst_frame_time_seconds = frame_time_seconds;
        }
    }

    const auto average_frame_time_ms =
        (total_frame_time_seconds / measured_frames) * 1000.0;
    const auto worst_frame_time_ms = worst_frame_time_seconds * 1000.0;

    std::printf(
        "TextRendering stress test: %d labels/frame (all changing), %d "
        "frames measured (after %d warmup) - average %.3f ms/frame, worst "
        "%.3f ms/frame\n",
        label_count,
        measured_frames,
        warmup_frames,
        average_frame_time_ms,
        worst_frame_time_ms
    );

    const auto success = average_frame_time_ms <= max_average_frame_time_ms;
    if (!success) {
        std::printf(
            "TextRendering stress test FAILED: average %.3f ms/frame "
            "exceeds threshold %.3f ms/frame\n",
            average_frame_time_ms,
            max_average_frame_time_ms
        );
    } else {
        std::printf("TextRendering stress test PASSED\n");
    }

    return success ? 0 : 1;
}
