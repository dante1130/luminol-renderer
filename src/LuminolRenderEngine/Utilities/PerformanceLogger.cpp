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

    // Every sample here (other than "frame" and "acquire_swapchain") times a
    // plain CPU std::chrono span around SDL_GPU command-recording calls, not
    // GPU execution: SDL_GPU submission is asynchronous, so these measure how
    // long the CPU took to encode commands, which is not the same as how
    // long the GPU took to run them. SDL_GPU has no timestamp/query API, so
    // there is currently no way to measure true per-pass GPU duration.
    //
    // "acquire_swapchain" (SDL_WaitAndAcquireGPUSwapchainTexture) is the one
    // place actual GPU cost differences are likely to surface: it blocks the
    // CPU until a swapchain image is free, which includes both
    // presentation/VSync pacing and the GPU catching up on backlogged work
    // from prior frames. Do not exclude it when comparing GPU-bound changes
    // between builds - excluding it (as an earlier version of this function
    // did) hides exactly the signal you'd be looking for.
    auto cpu_record_total_milliseconds = 0.0;

    for (const auto& sample : samples) {
        const auto average_seconds =
            sample.total_time / static_cast<double>(sample.count);
        const auto average_milliseconds =
            average_seconds.as<Units::Millisecond>().get_value();

        message += " " + sample.name + ": " +
                   std::to_string(average_milliseconds) + "ms |";

        if (sample.name != "acquire_swapchain" && sample.name != "frame") {
            cpu_record_total_milliseconds += average_milliseconds;
        }
    }

    message += " cpu_record_total: " +
        std::to_string(cpu_record_total_milliseconds) + "ms |";

    SDL_Log("%s", message.c_str());

    samples.clear();
    frame_count = 0;
}

}  // namespace Luminol::Utilities
