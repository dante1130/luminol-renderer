#pragma once

#include <functional>
#include <memory>

#include <gsl/gsl>
#include <SDL3/SDL_gpu.h>

namespace Luminol::Graphics::SDL_GPU {

class GPUDevice {
public:
    GPUDevice(SDL_Window* window);

    [[nodiscard]] auto get() const -> const SDL_GPUDevice&;
    [[nodiscard]] auto get() -> SDL_GPUDevice&;

private:
    using SDL_GPUDeviceDeleter = std::function<void(SDL_GPUDevice*)>;

    std::unique_ptr<SDL_GPUDevice, SDL_GPUDeviceDeleter> device;
};

}  // namespace Luminol::Graphics::SDL_GPU
