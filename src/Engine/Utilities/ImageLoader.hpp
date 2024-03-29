#pragma once

#include <filesystem>

namespace Luminol::Utilities::ImageLoader {

struct Image {
    std::vector<uint8_t> data = {};
    int32_t width = {0};
    int32_t height = {0};
    int32_t channels = {0};
};

auto load_image(const std::filesystem::path& path) -> Image;

}  // namespace Luminol::Utilities::ImageLoader
