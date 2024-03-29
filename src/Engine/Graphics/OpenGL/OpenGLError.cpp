#include "OpenGLError.hpp"

#include <format>
#include <iostream>

#include <glad/gl.h>

namespace {

constexpr auto source_to_string(uint32_t source) -> std::string_view {
    switch (source) {
        case GL_DEBUG_SOURCE_API:
            return "API";
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            return "Window System";
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            return "Shader Compiler";
        case GL_DEBUG_SOURCE_THIRD_PARTY:
            return "Third Party";
        case GL_DEBUG_SOURCE_APPLICATION:
            return "Application";
        case GL_DEBUG_SOURCE_OTHER:
            return "Other";
        default:
            return "Unknown";
    }
}

constexpr auto type_to_string(uint32_t type) -> std::string_view {
    switch (type) {
        case GL_DEBUG_TYPE_ERROR:
            return "Error";
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            return "Deprecated Behavior";
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            return "Undefined Behavior";
        case GL_DEBUG_TYPE_PORTABILITY:
            return "Portability";
        case GL_DEBUG_TYPE_PERFORMANCE:
            return "Performance";
        case GL_DEBUG_TYPE_MARKER:
            return "Marker";
        case GL_DEBUG_TYPE_PUSH_GROUP:
            return "Push Group";
        case GL_DEBUG_TYPE_POP_GROUP:
            return "Pop Group";
        case GL_DEBUG_TYPE_OTHER:
            return "Other";
        default:
            return "Unknown";
    }
}

constexpr auto severity_to_string(uint32_t severity) -> std::string_view {
    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:
            return "High";
        case GL_DEBUG_SEVERITY_MEDIUM:
            return "Medium";
        case GL_DEBUG_SEVERITY_LOW:
            return "Low";
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            return "Notification";
        default:
            return "Unknown";
    }
}

}  // namespace

namespace Luminol::Graphics {

auto APIENTRY gl_debug_message_callback(
    uint32_t source,
    uint32_t type,
    uint32_t /*identifier*/,
    uint32_t severity,
    int32_t /*length*/,
    const char* message,
    const void* /*user_param*/
) -> void {
    const auto source_str = source_to_string(source);
    const auto type_str = type_to_string(type);
    const auto severity_str = severity_to_string(severity);

    std::cerr << std::format(
        "OpenGL Debug Message\n"
        "  Source: {}\n"
        "  Type: {}\n"
        "  Severity: {}\n"
        "  Message: {}\n\n",
        source_str,
        type_str,
        severity_str,
        message
    );
}

}  // namespace Luminol::Graphics
