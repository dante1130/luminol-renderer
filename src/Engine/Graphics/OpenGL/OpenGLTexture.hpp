#pragma once

#include <filesystem>

namespace Luminol::Graphics {

class OpenGLTexture {
public:
    OpenGLTexture(const std::filesystem::path& path);
    ~OpenGLTexture();
    OpenGLTexture(const OpenGLTexture&) = default;
    OpenGLTexture(OpenGLTexture&&) = delete;
    auto operator=(const OpenGLTexture&) -> OpenGLTexture& = default;
    auto operator=(OpenGLTexture&&) -> OpenGLTexture& = delete;

    auto bind() const -> void;
    auto unbind() const -> void;

private:
    uint32_t texture_id = {0};
};

}  // namespace Luminol::Graphics
