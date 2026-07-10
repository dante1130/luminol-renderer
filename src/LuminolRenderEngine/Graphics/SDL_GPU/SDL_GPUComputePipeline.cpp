#include "SDL_GPUComputePipeline.hpp"

#include <gsl/gsl>

#include <SDL3/SDL_gpu.h>

namespace Luminol::Graphics::SDL_GPU {

ComputePipeline::ComputePipeline(
    std::unique_ptr<SDL_GPUComputePipeline, SDL_GPUComputePipelineDeleter>
        pipeline
)
    : pipeline{std::move(pipeline)} {
    Expects(this->pipeline);
}

auto ComputePipeline::native_handle() const -> SDL_GPUComputePipeline* {
    return pipeline.get();
}

}  // namespace Luminol::Graphics::SDL_GPU
