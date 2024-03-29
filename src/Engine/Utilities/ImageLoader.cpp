#include "ImageLoader.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace Luminol::Utilities::ImageLoader {

auto load_image(const std::filesystem::path& path) -> Image {
    Image image = {};

    stbi_set_flip_vertically_on_load(1);
    auto* image_data = stbi_load(
        path.string().c_str(), &image.width, &image.height, &image.channels, 0
    );

    image.data.resize(image.width * image.height * image.channels);
    std::memcpy(
        image.data.data(), image_data, image.data.size() * sizeof(uint8_t)
    );

    stbi_image_free(image_data);

    return image;
}

}  // namespace Luminol::Utilities::ImageLoader
