#pragma once

#include <string_view>
#include <unordered_map>
#include <vector>

#include <LuminolMaths/Vector.hpp>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUFont.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUGraphicsPipeline.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUShader.hpp>

struct SDL_Window;

namespace Luminol::Graphics::SDL_GPU {

class GPUDevice;
class CommandBuffer;
class RenderPass;

// Renders screen-space text as alpha-blended quads sampling each font's
// pre-baked glyph atlas (see SDL_GPUFont) on top of the final tonemapped
// image. Unlike a per-string-texture design, no GPU resources are
// allocated per frame - queue_draw only does CPU quad-geometry math, and
// flush_frame_geometry uploads one small vertex buffer per font used that
// frame.
class SDL_GPUTextRenderPass {
public:
    SDL_GPUTextRenderPass(GPUDevice& device, SDL_Window* window);

    // CPU-only: looks up glyph quads from font's atlas and appends them to
    // this frame's pending geometry. No GPU device/command-buffer work,
    // since no command buffer exists yet at the point application code
    // calls this (before draw()). See flush_frame_geometry.
    auto queue_draw(
        FontId font_id,
        const SDL_GPUFont& font,
        std::string_view text,
        const Maths::Vector2f& position,
        const Maths::Vector4f& color
    ) -> void;

    // Uploads this frame's queued glyph geometry (one vertex buffer per
    // font used), batched into one copy pass on the caller's
    // command_buffer. Must run before any render pass is opened on
    // command_buffer this frame, and before draw() below.
    auto flush_frame_geometry(GPUDevice& device, CommandBuffer& command_buffer)
        -> void;

    auto draw(
        CommandBuffer& command_buffer,
        RenderPass& render_pass,
        uint32_t screen_width,
        uint32_t screen_height
    ) -> void;

private:
    struct TextVertex {
        Maths::Vector2f position;
        Maths::Vector2f uv;
        Maths::Vector4f color;
    };

    struct DrawBatch {
        const SDL_GPUFont* font = nullptr;
        Buffer vertex_buffer;
        uint32_t vertex_count = 0;
    };

    Shader text_vertex_shader;
    Shader text_fragment_shader;
    GraphicsPipeline text_pipeline;

    std::unordered_map<FontId, std::vector<TextVertex>> pending_glyphs;
    std::unordered_map<FontId, const SDL_GPUFont*> pending_fonts;
    std::vector<DrawBatch> draw_batches;
};

}  // namespace Luminol::Graphics::SDL_GPU
