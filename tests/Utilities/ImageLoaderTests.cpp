#include <LuminolRenderEngine/Utilities/ImageLoader.hpp>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

using namespace Luminol::Utilities::ImageLoader;

TEST_CASE("get_default_normal_map returns a 1x1 flat-normal RGBA pixel") {
    const auto image = get_default_normal_map();

    CHECK(image.width == 1);
    CHECK(image.height == 1);
    CHECK(image.channels == 4);
    REQUIRE(image.data.size() == 4);
    CHECK(image.data[0] == 0x7F);
    CHECK(image.data[1] == 0x7F);
    CHECK(image.data[2] == 0xFF);
    CHECK(image.data[3] == 0xFF);
}

TEST_CASE("load_image decodes a small fixture with a consistent data size") {
    const auto image = load_image("res/textures/floor_albedo.png");

    CHECK(image.width > 0);
    CHECK(image.height > 0);
    CHECK(image.channels > 0);
    CHECK(
        image.data.size() ==
        static_cast<size_t>(image.width) *
            static_cast<size_t>(image.height) *
            static_cast<size_t>(image.channels)
    );
}

TEST_CASE("load_image with desired_channels forces the channel count") {
    const auto image = load_image("res/textures/floor_metallic_roughness.png", 4);

    CHECK(image.channels == 4);
    CHECK(
        image.data.size() ==
        static_cast<size_t>(image.width) * static_cast<size_t>(image.height) * 4
    );
}
