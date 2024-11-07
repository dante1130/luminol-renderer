#include "ImageLoader.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace Luminol::Utilities::ImageLoader {

auto load_image(const std::filesystem::path& path) -> Image {
    Image image = {.path = path, .data = {}};

    auto* image_data = stbi_load(
        path.string().c_str(), &image.width, &image.height, &image.channels, 0
    );

    image.data.resize(
        static_cast<size_t>(image.width) * image.height * image.channels
    );
    std::memcpy(
        image.data.data(), image_data, image.data.size() * sizeof(uint8_t)
    );

    stbi_image_free(image_data);

    return image;
}

auto get_default_normal_map() -> Image {
    const auto default_normal_map_pixels =
        std::vector<uint8_t>{0x7F, 0x7F, 0xFF, 0xFF};

    return Image{
        .path = {},
        .data = default_normal_map_pixels,
        .width = 1,
        .height = 1,
        .channels = 4,
    };
}

}  // namespace Luminol::Utilities::ImageLoader
