#include "SDL_GPURenderPass.hpp"

#include <gsl/gsl>

#include <SDL3/SDL_gpu.h>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUGraphicsPipeline.hpp>

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

auto RenderPass::bind_graphics_pipeline(const GraphicsPipeline& pipeline)
    -> void {
    Expects(render_pass != nullptr);
    SDL_BindGPUGraphicsPipeline(render_pass, pipeline.native_handle());
}

auto RenderPass::draw_primitives(
    uint32_t num_vertices,
    uint32_t num_instances,
    uint32_t first_vertex,
    uint32_t first_instance
) -> void {
    Expects(render_pass != nullptr);
    SDL_DrawGPUPrimitives(
        render_pass,
        num_vertices,
        num_instances,
        first_vertex,
        first_instance
    );
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
