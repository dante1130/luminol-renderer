#pragma once

#include <LuminolRenderEngine/Graphics/SkyboxPaths.hpp>

namespace Luminol::Graphics {

class OpenGLSkybox {
public:
    OpenGLSkybox(const SkyboxPaths& paths);

    auto bind() const -> void;
    auto unbind() const -> void;

private:
    uint32_t skybox_texture_id = {0};
};

}  // namespace Luminol::Graphics
