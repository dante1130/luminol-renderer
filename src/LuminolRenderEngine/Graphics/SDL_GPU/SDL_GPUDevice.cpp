#pragma once

#include "SDL_GPUDevice.hpp"

#include <gsl/gsl>

#include <SDL3/SDL_log.h>

namespace {

auto create_sdl_gpu_device(SDL_Window* window) -> SDL_GPUDevice* {
    auto* gpu_device =
        SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, nullptr);

    if (gpu_device == nullptr) {
        SDL_LogError(
            SDL_LOG_CATEGORY_ERROR,
            "Failed to create SDL_GPUDevice: %s",
            SDL_GetError()
        );
        return nullptr;
    }

    if (!SDL_ClaimWindowForGPUDevice(gpu_device, window)) {
        SDL_LogError(
            SDL_LOG_CATEGORY_ERROR,
            "Failed to claim window for SDL_GPUDevice: %s",
            SDL_GetError()
        );
        return nullptr;
    }

    return gpu_device;
}

}  // namespace

namespace Luminol::Graphics::SDL_GPU {

GPUDevice::GPUDevice(SDL_Window* window)
    : window{window},
      device{create_sdl_gpu_device(window), [&window](SDL_GPUDevice* device) {
                 SDL_ReleaseWindowFromGPUDevice(device, window);
                 SDL_DestroyGPUDevice(device);
             }} {
    Expects(this->window);
    Expects(this->device);
}

}  // namespace Luminol::Graphics::SDL_GPU
