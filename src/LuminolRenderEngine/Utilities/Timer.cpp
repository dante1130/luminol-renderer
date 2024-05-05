#include "Timer.hpp"

namespace Luminol::Utilities {

Timer::Timer() : start_time(std::chrono::high_resolution_clock::now()) {}

auto Timer::elapsed_seconds() const -> double {
    const auto end_time = std::chrono::high_resolution_clock::now();
    const auto elapsed_time = end_time - this->start_time;

    return std::chrono::duration<double>(elapsed_time).count();
}

}  // namespace Luminol::Utilities
