#pragma once

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTexture.hpp>
#include <LuminolRenderEngine/Graphics/SkyboxPaths.hpp>

namespace Luminol::Graphics::SDL_GPU {

class GPUDevice;
class CopyPass;

// Owns a cubemap texture holding the 6 faces of a skybox, uploaded via a
// caller-provided CopyPass (see SDL_GPUSkyboxRenderPass for the one-shot
// command buffer/copy pass used to construct this at renderer setup time).
class SDL_GPUSkybox {
public:
    SDL_GPUSkybox(
        GPUDevice& device, CopyPass& copy_pass, const SkyboxPaths& paths
    );

    [[nodiscard]] auto get_texture() const -> const Texture&;
    [[nodiscard]] auto get_sampler() const -> const Sampler&;

private:
    Texture texture;
    Sampler sampler;
};

}  // namespace Luminol::Graphics::SDL_GPU
