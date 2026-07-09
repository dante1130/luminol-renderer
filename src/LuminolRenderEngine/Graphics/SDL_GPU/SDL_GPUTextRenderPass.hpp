#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <LuminolMaths/Vector.hpp>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUFont.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUGraphicsPipeline.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUShader.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTexture.hpp>

struct SDL_Window;

namespace Luminol::Graphics::SDL_GPU {

class GPUDevice;
class CommandBuffer;
class RenderPass;

// Renders screen-space text as alpha-blended quads on top of the final
// tonemapped image. Each unique (font, text, color) string is rasterized via
// SDL_ttf once and cached as a GPU texture; unchanged strings are reused
// frame to frame, and entries not requested in a frame are evicted at the end
// of that frame so strings that change often (e.g. an FPS counter) don't grow
// the cache without bound.
class SDL_GPUTextRenderPass {
public:
    SDL_GPUTextRenderPass(GPUDevice& device, SDL_Window* window);

    auto queue_draw(
        GPUDevice& device,
        FontId font_id,
        const SDL_GPUFont& font,
        std::string_view text,
        const Maths::Vector2f& position,
        const Maths::Vector4f& color
    ) -> void;

    auto draw(
        CommandBuffer& command_buffer,
        RenderPass& render_pass,
        uint32_t screen_width,
        uint32_t screen_height
    ) -> void;

private:
    struct TextCacheEntry {
        Texture texture;
        uint32_t width;
        uint32_t height;
        bool used_this_frame = false;
    };

    struct QueuedText {
        const TextCacheEntry* entry;
        Maths::Vector2f position;
    };

    Shader text_vertex_shader;
    Shader text_fragment_shader;
    GraphicsPipeline text_pipeline;

    Sampler clamp_sampler;

    std::unordered_map<std::string, TextCacheEntry> text_cache;
    std::vector<QueuedText> queued_draws;
};

}  // namespace Luminol::Graphics::SDL_GPU
