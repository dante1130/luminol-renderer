#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include <unordered_map>

#include <LuminolRenderEngine/Graphics/RenderableManager.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTexture.hpp>

struct TTF_Font;

namespace Luminol::Graphics::SDL_GPU {

using Luminol::Graphics::FontId;

class GPUDevice;
class CopyPass;

// One printable-ASCII glyph's location within the font's atlas texture
// (u0,v0,u1,v1: normalized UV rect) and layout metrics (in pixels, relative
// to the pen position at the glyph's baseline origin) needed to position its
// quad.
struct GlyphInfo {
    float u0;
    float v0;
    float u1;
    float v1;
    float width;
    float height;
    float bearing_x;
    float bearing_y;
    float advance;
};

// Owns a loaded TTF_Font plus a GPU texture atlas containing every printable
// ASCII glyph (32-126), baked once at construction. Rendering text never
// rasterizes or allocates GPU resources per-frame - see
// SDL_GPUTextRenderPass, which only samples this atlas.
class SDL_GPUFont {
public:
    using TTF_FontDeleter = std::function<void(TTF_Font*)>;

    SDL_GPUFont(
        GPUDevice& device,
        CopyPass& copy_pass,
        const std::filesystem::path& font_path,
        float point_size
    );

    [[nodiscard]] auto native_handle() const -> TTF_Font*;

    // Returns nullptr for characters outside the supported printable-ASCII
    // (32-126) range.
    [[nodiscard]] auto get_glyph(char32_t codepoint) const
        -> const GlyphInfo*;

    [[nodiscard]] auto get_atlas_texture() const -> const Texture&;
    [[nodiscard]] auto get_atlas_sampler() const -> const Sampler&;

    // Distance in pixels from a text line's top to its baseline; used to
    // position glyph quads relative to the "position = top-left of text"
    // convention SDL_GPUTextRenderPass::queue_draw's callers use.
    [[nodiscard]] auto get_ascent() const -> float;

private:
    std::unique_ptr<TTF_Font, TTF_FontDeleter> font;

    // Declared before atlas_texture: atlas_texture's initializer (in the
    // .cpp) populates this via `this->glyphs = ...`, which requires glyphs
    // to already be constructed - members initialize in declaration order
    // regardless of member-initializer-list order.
    std::unordered_map<char32_t, GlyphInfo> glyphs;
    Texture atlas_texture;
    Sampler atlas_sampler;
    float ascent;
};

}  // namespace Luminol::Graphics::SDL_GPU
