#pragma once

#include <filesystem>

namespace Luminol::Graphics {

struct SkyboxPaths {
    std::filesystem::path front;
    std::filesystem::path back;
    std::filesystem::path top;
    std::filesystem::path bottom;
    std::filesystem::path left;
    std::filesystem::path right;
};

}  // namespace Luminol::Graphics
