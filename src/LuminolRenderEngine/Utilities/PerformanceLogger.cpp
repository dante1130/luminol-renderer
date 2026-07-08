#include "PerformanceLogger.hpp"

#include <algorithm>

#include <SDL3/SDL_log.h>

namespace Luminol::Utilities {

PerformanceLogger::PerformanceLogger(uint32_t log_interval_frames)
    : log_interval_frames{log_interval_frames} {}

auto PerformanceLogger::record(std::string_view name, Units::Seconds elapsed)
    -> void {
    const auto existing = std::find_if(
        samples.begin(),
        samples.end(),
        [name](const Sample& sample) { return sample.name == name; }
    );

    if (existing != samples.end()) {
        existing->total_time += elapsed;
        existing->count += 1;
        return;
    }

    samples.push_back(Sample{
        .name = std::string{name},
        .total_time = elapsed,
        .count = 1,
    });
}

auto PerformanceLogger::end_frame() -> void {
    frame_count += 1;

    if (frame_count < log_interval_frames) {
        return;
    }

    log_and_reset();
}

auto PerformanceLogger::log_and_reset() -> void {
    auto message = std::string{"[Perf]"};

    for (const auto& sample : samples) {
        const auto average_seconds =
            sample.total_time / static_cast<double>(sample.count);
        const auto average_milliseconds =
            average_seconds.as<Units::Millisecond>().get_value();

        message += " " + sample.name + ": " +
                   std::to_string(average_milliseconds) + "ms |";
    }

    SDL_Log("%s", message.c_str());

    samples.clear();
    frame_count = 0;
}

}  // namespace Luminol::Utilities
