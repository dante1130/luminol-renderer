#include "SDL_GPUFont.hpp"

#include <algorithm>
#include <cstring>
#include <optional>
#include <tuple>
#include <vector>

#include <gsl/gsl>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_surface.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCopyPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUDevice.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTransferBuffer.hpp>

namespace {

using namespace Luminol::Graphics::SDL_GPU;

auto font_deleter(TTF_Font* font) -> void { TTF_CloseFont(font); }

// Printable ASCII only - matches this codebase's current text usage (no
// Unicode/extended glyph support).
constexpr auto first_printable_codepoint = char32_t{32};
constexpr auto last_printable_codepoint = char32_t{126};

constexpr auto atlas_columns = uint32_t{10};
constexpr auto cell_padding_pixels = uint32_t{2};

struct RasterizedGlyph {
    char32_t codepoint;
    std::vector<uint8_t> pixels;  // RGBA32, tightly packed, may be empty
    uint32_t width;               // 0 for glyphs with no ink (e.g. space)
    uint32_t height;
    float bearing_x;
    float bearing_y;
    float advance;
};

auto rasterize_glyph(TTF_Font* font, char32_t codepoint)
    -> std::optional<RasterizedGlyph> {
    if (!TTF_FontHasGlyph(font, codepoint)) {
        return std::nullopt;
    }

    auto minx = int{};
    auto maxx = int{};
    auto miny = int{};
    auto maxy = int{};
    auto advance = int{};
    if (!TTF_GetGlyphMetrics(
            font, codepoint, &minx, &maxx, &miny, &maxy, &advance
        )) {
        return std::nullopt;
    }

    auto glyph = RasterizedGlyph{
        .codepoint = codepoint,
        .pixels = {},
        .width = 0,
        .height = 0,
        .bearing_x = static_cast<float>(minx),
        .bearing_y = static_cast<float>(-maxy),
        .advance = static_cast<float>(advance),
    };

    auto image_type = TTF_ImageType{};
    auto* rendered_surface = TTF_GetGlyphImage(font, codepoint, &image_type);
    if (rendered_surface == nullptr) {
        // No ink (e.g. space) - keep the advance/bearing, empty bitmap.
        return glyph;
    }

    auto* rgba_surface =
        SDL_ConvertSurface(rendered_surface, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(rendered_surface);
    if (rgba_surface == nullptr) {
        return glyph;
    }

    glyph.width = static_cast<uint32_t>(rgba_surface->w);
    glyph.height = static_cast<uint32_t>(rgba_surface->h);
    const auto row_bytes = glyph.width * 4U;
    glyph.pixels.resize(static_cast<size_t>(row_bytes) * glyph.height);

    const auto* src_pixels =
        static_cast<const uint8_t*>(rgba_surface->pixels);
    for (uint32_t row = 0; row < glyph.height; ++row) {
        std::memcpy(
            glyph.pixels.data() + (row * row_bytes),
            src_pixels + (row * static_cast<uint32_t>(rgba_surface->pitch)),
            row_bytes
        );
    }

    SDL_DestroySurface(rgba_surface);

    return glyph;
}

auto build_atlas(
    GPUDevice& device,
    CopyPass& copy_pass,
    const std::vector<RasterizedGlyph>& rasterized_glyphs
) -> std::tuple<Texture, std::unordered_map<char32_t, GlyphInfo>> {
    auto max_cell_width = uint32_t{1};
    auto max_cell_height = uint32_t{1};
    for (const auto& glyph : rasterized_glyphs) {
        max_cell_width = std::max(max_cell_width, glyph.width);
        max_cell_height = std::max(max_cell_height, glyph.height);
    }

    const auto cell_width = max_cell_width + cell_padding_pixels;
    const auto cell_height = max_cell_height + cell_padding_pixels;
    const auto rows =
        (static_cast<uint32_t>(rasterized_glyphs.size()) + atlas_columns - 1U) /
        atlas_columns;
    const auto atlas_width = atlas_columns * cell_width;
    const auto atlas_height = std::max(rows, 1U) * cell_height;
    const auto atlas_row_bytes = atlas_width * 4U;

    auto atlas_pixels =
        std::vector<uint8_t>(static_cast<size_t>(atlas_row_bytes) * atlas_height, 0);

    auto glyphs = std::unordered_map<char32_t, GlyphInfo>{};
    glyphs.reserve(rasterized_glyphs.size());

    for (size_t index = 0; index < rasterized_glyphs.size(); ++index) {
        const auto& glyph = rasterized_glyphs.at(index);
        const auto column = static_cast<uint32_t>(index) % atlas_columns;
        const auto row = static_cast<uint32_t>(index) / atlas_columns;
        const auto dest_x = column * cell_width;
        const auto dest_y = row * cell_height;

        for (uint32_t pixel_row = 0; pixel_row < glyph.height; ++pixel_row) {
            const auto* src_row =
                glyph.pixels.data() + (pixel_row * glyph.width * 4U);
            auto* dest_row = atlas_pixels.data() +
                (((dest_y + pixel_row) * atlas_row_bytes) + (dest_x * 4U));
            std::memcpy(dest_row, src_row, glyph.width * 4U);
        }

        glyphs.emplace(
            glyph.codepoint,
            GlyphInfo{
                .u0 = static_cast<float>(dest_x) / static_cast<float>(atlas_width),
                .v0 = static_cast<float>(dest_y) / static_cast<float>(atlas_height),
                .u1 = static_cast<float>(dest_x + glyph.width) /
                    static_cast<float>(atlas_width),
                .v1 = static_cast<float>(dest_y + glyph.height) /
                    static_cast<float>(atlas_height),
                .width = static_cast<float>(glyph.width),
                .height = static_cast<float>(glyph.height),
                .bearing_x = glyph.bearing_x,
                .bearing_y = glyph.bearing_y,
                .advance = glyph.advance,
            }
        );
    }

    auto atlas_texture = device.create_texture(TextureInfo{
        .width = atlas_width,
        .height = atlas_height,
        .format = TextureFormat::R8G8B8A8_Unorm,
    });

    auto transfer_buffer = device.create_transfer_buffer(TransferBufferInfo{
        .usage = TransferBufferUsage::Upload,
        .size = static_cast<uint32_t>(atlas_pixels.size()),
    });
    {
        const auto mapped = transfer_buffer.map(false);
        std::memcpy(mapped.data(), atlas_pixels.data(), atlas_pixels.size());
        transfer_buffer.unmap();
    }

    copy_pass.upload_to_texture(
        transfer_buffer, 0, atlas_texture, atlas_width, atlas_height, false
    );

    return {std::move(atlas_texture), std::move(glyphs)};
}

}  // namespace

namespace Luminol::Graphics::SDL_GPU {

SDL_GPUFont::SDL_GPUFont(
    GPUDevice& device,
    CopyPass& copy_pass,
    const std::filesystem::path& font_path,
    float point_size
)
    : font{
          TTF_OpenFont(font_path.string().c_str(), point_size), font_deleter
      },
      atlas_texture{[&] {
          Expects(this->font);
          auto rasterized_glyphs = std::vector<RasterizedGlyph>{};
          for (auto codepoint = first_printable_codepoint;
               codepoint <= last_printable_codepoint; ++codepoint) {
              if (auto glyph = rasterize_glyph(this->font.get(), codepoint);
                  glyph.has_value()) {
                  rasterized_glyphs.push_back(std::move(glyph.value()));
              }
          }
          auto [texture, glyph_map] =
              build_atlas(device, copy_pass, rasterized_glyphs);
          this->glyphs = std::move(glyph_map);
          return texture;
      }()},
      atlas_sampler{device.create_sampler(SamplerInfo{
          .filter = SamplerFilter::Linear,
          .address_mode_u = SamplerAddressMode::ClampToEdge,
          .address_mode_v = SamplerAddressMode::ClampToEdge,
      })},
      ascent{static_cast<float>(TTF_GetFontAscent(this->font.get()))} {
    Expects(this->font);
}

auto SDL_GPUFont::native_handle() const -> TTF_Font* { return font.get(); }

auto SDL_GPUFont::get_glyph(char32_t codepoint) const -> const GlyphInfo* {
    const auto found = glyphs.find(codepoint);
    return found == glyphs.end() ? nullptr : &found->second;
}

auto SDL_GPUFont::get_atlas_texture() const -> const Texture& {
    return atlas_texture;
}

auto SDL_GPUFont::get_atlas_sampler() const -> const Sampler& {
    return atlas_sampler;
}

auto SDL_GPUFont::get_ascent() const -> float { return ascent; }

}  // namespace Luminol::Graphics::SDL_GPU
