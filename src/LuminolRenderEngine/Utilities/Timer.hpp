#pragma once

#include <chrono>

namespace Luminol::Utilities {

class Timer {
public:
    Timer();

    [[nodiscard]] auto elapsed_seconds() const -> double;

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
};

}  // namespace Luminol::Utilities
