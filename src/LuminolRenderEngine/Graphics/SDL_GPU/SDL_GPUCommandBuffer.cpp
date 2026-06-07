#include "SDL_GPUCommandBuffer.hpp"

#include <gsl/gsl>

#include <SDL3/SDL_log.h>

namespace Luminol::Graphics::SDL_GPU {

Luminol::Graphics::SDL_GPU::CommandBuffer::CommandBuffer(SDL_GPUCommandBuffer* command_buffer) 
	: command_buffer{command_buffer} {
    Expects(this->command_buffer != nullptr);
}

}  // namespace Luminol::Graphics::SDL_GPU
