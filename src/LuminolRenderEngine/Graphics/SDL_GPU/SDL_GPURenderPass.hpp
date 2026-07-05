#pragma once

struct SDL_GPURenderPass;

namespace Luminol::Graphics::SDL_GPU {

class RenderPass {
public:
    RenderPass(SDL_GPURenderPass* render_pass);
    ~RenderPass();

    RenderPass(const RenderPass&) = delete;
    auto operator=(const RenderPass&) -> RenderPass& = delete;

    RenderPass(RenderPass&& other) noexcept;
    auto operator=(RenderPass&& other) noexcept -> RenderPass&;

    [[nodiscard]] auto native_handle() const -> SDL_GPURenderPass*;

private:
    auto end() noexcept -> void;

    SDL_GPURenderPass* render_pass;
};

}  // namespace Luminol::Graphics::SDL_GPU
