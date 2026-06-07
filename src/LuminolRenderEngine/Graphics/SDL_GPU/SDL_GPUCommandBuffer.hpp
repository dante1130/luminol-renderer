#pragma once

#include <SDL3/SDL_gpu.h>

namespace Luminol::Graphics::SDL_GPU {

class CommandBuffer {
public:
    CommandBuffer(SDL_GPUCommandBuffer* command_buffer);

private:
    SDL_GPUCommandBuffer* command_buffer;
};

}  // namespace Luminol::Graphics::SDL_GPU

