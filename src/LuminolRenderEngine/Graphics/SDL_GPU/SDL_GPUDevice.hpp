#pragma once

#include <functional>
#include <memory>

#include <gsl/gsl>
#include <SDL3/SDL_gpu.h>

namespace Luminol::Graphics::SDL_GPU {

class GPUDevice {
public:
    GPUDevice(SDL_Window* window);

private:
    using SDL_GPUDeviceDeleter = std::function<void(SDL_GPUDevice*)>;

    SDL_Window* window = nullptr;
    std::unique_ptr<SDL_GPUDevice, SDL_GPUDeviceDeleter> device;
};

}  // namespace Luminol::Graphics::SDL_GPU
