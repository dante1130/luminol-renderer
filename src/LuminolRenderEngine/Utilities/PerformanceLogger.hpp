#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <LuminolMaths/Units/Time.hpp>

namespace Luminol::Utilities {

class PerformanceLogger {
public:
    constexpr static auto default_log_interval_frames = uint32_t{120};

    explicit PerformanceLogger(
        uint32_t log_interval_frames = default_log_interval_frames
    );

    auto record(std::string_view name, Units::Seconds elapsed) -> void;
    auto end_frame() -> void;

private:
    struct Sample {
        std::string name;
        Units::Seconds total_time{0.0};
        uint32_t count = 0;
    };

    auto log_and_reset() -> void;

    std::vector<Sample> samples;
    uint32_t frame_count = 0;
    uint32_t log_interval_frames;
};

}  // namespace Luminol::Utilities
