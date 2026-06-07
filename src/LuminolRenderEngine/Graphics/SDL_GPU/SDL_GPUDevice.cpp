#pragma once

#include "SDL_GPUDevice.hpp"

#include <fstream>

#include <gsl/gsl>

#include <SDL3/SDL_log.h>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCommandBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUShader.hpp>

namespace {

using namespace Luminol::Graphics::SDL_GPU;

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

constexpr auto luminol_shader_stage_to_sdl_gpu_shader_stage(ShaderStage stage)
    -> SDL_GPUShaderStage {
    switch (stage) {
        case ShaderStage::Vertex:
            return SDL_GPU_SHADERSTAGE_VERTEX;
        case ShaderStage::Fragment:
            return SDL_GPU_SHADERSTAGE_FRAGMENT;
        default:
            throw std::runtime_error{"Invalid shader stage"};
    }
}

}  // namespace

namespace Luminol::Graphics::SDL_GPU {

GPUDevice::GPUDevice(SDL_Window* window)
    : device{create_sdl_gpu_device(window), [&window](SDL_GPUDevice* device) {
                 SDL_ReleaseWindowFromGPUDevice(device, window);
                 SDL_DestroyGPUDevice(device);
             }} {
    Expects(this->device != nullptr);
}

auto GPUDevice::create_command_buffer() -> CommandBuffer {
	auto* command_buffer = SDL_AcquireGPUCommandBuffer(device.get());

	if (command_buffer == nullptr) {
		SDL_LogError(
			SDL_LOG_CATEGORY_ERROR,
			"Failed to create SDL_GPUCommandBuffer: %s",
			SDL_GetError()
		);
		return nullptr;
	}

	return CommandBuffer{command_buffer};
}

auto GPUDevice::create_shader(const ShaderInfo& info)
    -> Shader {
    auto shader_file = std::ifstream{
        info.path, std::ios::in | std::ios::binary | std::ios::ate
    };

    const auto code_size = shader_file.tellg();
    shader_file.seekg(0, std::ios::beg);

    auto code_vector = std::vector<uint8_t>{};
    code_vector.resize(code_size);
    shader_file.read(reinterpret_cast<char*>(code_vector.data()), code_size);

    const auto sdl_gpu_shader_create_info = SDL_GPUShaderCreateInfo{
        .code_size = static_cast<size_t>(code_size),
        .code = code_vector.data(),
        .entrypoint = "main",
        .format = SDL_GPU_SHADERFORMAT_SPIRV,
        .stage = luminol_shader_stage_to_sdl_gpu_shader_stage(info.stage),
        .num_samplers = info.sampler_count,
        .num_storage_textures = info.storage_texture_count,
        .num_storage_buffers = info.storage_buffer_count,
        .num_uniform_buffers = info.uniform_buffer_count,
        .props = 0,
    };

    return std::unique_ptr<SDL_GPUShader, Shader::SDL_GPUShaderDeleter>(
        SDL_CreateGPUShader(this->device.get(), &sdl_gpu_shader_create_info),
        [this](SDL_GPUShader* shader) { SDL_ReleaseGPUShader(this->device.get(), shader); }
    );
}

}  // namespace Luminol::Graphics::SDL_GPU
