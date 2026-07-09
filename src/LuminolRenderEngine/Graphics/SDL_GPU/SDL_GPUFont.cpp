#include "SDL_GPUFont.hpp"

#include <gsl/gsl>

#include <SDL3_ttf/SDL_ttf.h>

namespace {

auto font_deleter(TTF_Font* font) -> void { TTF_CloseFont(font); }

}  // namespace

namespace Luminol::Graphics::SDL_GPU {

SDL_GPUFont::SDL_GPUFont(
    const std::filesystem::path& font_path, float point_size
)
    : font{
          TTF_OpenFont(font_path.string().c_str(), point_size), font_deleter
      } {
    Expects(this->font);
}

auto SDL_GPUFont::native_handle() const -> TTF_Font* { return font.get(); }

}  // namespace Luminol::Graphics::SDL_GPU
