#include "SDL_GPURenderPass.hpp"

#include <gsl/gsl>

#include <SDL3/SDL_gpu.h>

namespace Luminol::Graphics::SDL_GPU {

RenderPass::RenderPass(SDL_GPURenderPass* render_pass)
    : render_pass{render_pass} {
    Expects(this->render_pass != nullptr);
}

RenderPass::~RenderPass() { end(); }

RenderPass::RenderPass(RenderPass&& other) noexcept
    : render_pass{other.render_pass} {
    other.render_pass = nullptr;
}

auto RenderPass::operator=(RenderPass&& other) noexcept -> RenderPass& {
    if (this != &other) {
        end();
        render_pass = other.render_pass;
        other.render_pass = nullptr;
    }
    return *this;
}

auto RenderPass::native_handle() const -> SDL_GPURenderPass* {
    return render_pass;
}

auto RenderPass::end() noexcept -> void {
    if (render_pass != nullptr) {
        SDL_EndGPURenderPass(render_pass);
        render_pass = nullptr;
    }
}

}  // namespace Luminol::Graphics::SDL_GPU
