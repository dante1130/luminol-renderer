#pragma once

#include <cstdint>

#ifdef _WIN32
#include <windows.h>
#else
#define APIENTRY
#endif

namespace Luminol::Graphics {

auto APIENTRY gl_debug_message_callback(
    uint32_t source,
    uint32_t type,
    uint32_t identifier,
    uint32_t severity,
    int32_t length,
    const char* message,
    const void* user_param
) -> void;

}  // namespace Luminol::Graphics
