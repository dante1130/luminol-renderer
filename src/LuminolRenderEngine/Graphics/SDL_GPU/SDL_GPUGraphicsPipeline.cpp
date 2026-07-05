#include "SDL_GPUGraphicsPipeline.hpp"

#include <gsl/gsl>

namespace Luminol::Graphics::SDL_GPU {

GraphicsPipeline::GraphicsPipeline(
    std::unique_ptr<SDL_GPUGraphicsPipeline, SDL_GPUGraphicsPipelineDeleter>
        pipeline
)
    : pipeline{std::move(pipeline)} {
    Expects(this->pipeline);
}

auto GraphicsPipeline::get() const -> SDL_GPUGraphicsPipeline* {
    return pipeline.get();
}

}  // namespace Luminol::Graphics::SDL_GPU
