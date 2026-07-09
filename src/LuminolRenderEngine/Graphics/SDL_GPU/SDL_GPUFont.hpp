#pragma once

#include <filesystem>
#include <functional>
#include <memory>

#include <LuminolRenderEngine/Graphics/RenderableManager.hpp>

struct TTF_Font;

namespace Luminol::Graphics::SDL_GPU {

using Luminol::Graphics::FontId;

class SDL_GPUFont {
public:
    using TTF_FontDeleter = std::function<void(TTF_Font*)>;

    SDL_GPUFont(const std::filesystem::path& font_path, float point_size);

    [[nodiscard]] auto native_handle() const -> TTF_Font*;

private:
    std::unique_ptr<TTF_Font, TTF_FontDeleter> font;
};

}  // namespace Luminol::Graphics::SDL_GPU
