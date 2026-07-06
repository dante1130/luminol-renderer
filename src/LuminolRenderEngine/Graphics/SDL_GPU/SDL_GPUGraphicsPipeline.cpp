#include "SDL_GPUGraphicsPipeline.hpp"

#include <gsl/gsl>

#include <SDL3/SDL_gpu.h>

namespace Luminol::Graphics::SDL_GPU {

GraphicsPipeline::GraphicsPipeline(
    std::unique_ptr<SDL_GPUGraphicsPipeline, SDL_GPUGraphicsPipelineDeleter>
        pipeline
)
    : pipeline{std::move(pipeline)} {
    Expects(this->pipeline);
}

auto GraphicsPipeline::native_handle() const -> SDL_GPUGraphicsPipeline* {
    return pipeline.get();
}

}  // namespace Luminol::Graphics::SDL_GPU
