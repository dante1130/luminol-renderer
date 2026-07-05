#pragma once

#include "SDL_GPUDevice.hpp"

#include <fstream>

#include <gsl/gsl>

#include <SDL3/SDL_log.h>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCommandBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUShader.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUGraphicsPipeline.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTransferBuffer.hpp>

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
    : device{create_sdl_gpu_device(window), [window](SDL_GPUDevice* device) {
                 SDL_ReleaseWindowFromGPUDevice(device, window);
                 SDL_DestroyGPUDevice(device);
             }} {
    Expects(this->device != nullptr);

    const auto swapchain_format =
        SDL_GetGPUSwapchainTextureFormat(this->device.get(), window);
    if (swapchain_format == SDL_GPU_TEXTUREFORMAT_INVALID) {
        SDL_LogError(
            SDL_LOG_CATEGORY_ERROR,
            "Window is not claimed by GPU device after claim call: %s",
            SDL_GetError()
        );
        Ensures(false);
    }
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

auto GPUDevice::create_graphics_pipeline(const GraphicsPipelineInfo& info)
    -> GraphicsPipeline {
    const auto color_target_description = SDL_GPUColorTargetDescription{
        .format = info.color_target_format,
        .blend_state = {},
    };

    const auto create_info = SDL_GPUGraphicsPipelineCreateInfo{
        .vertex_shader = info.vertex_shader.get(),
        .fragment_shader = info.fragment_shader.get(),
        .vertex_input_state =
            SDL_GPUVertexInputState{
                .vertex_buffer_descriptions =
                    info.vertex_buffer_descriptions.data(),
                .num_vertex_buffers = static_cast<uint32_t>(
                    info.vertex_buffer_descriptions.size()
                ),
                .vertex_attributes = info.vertex_attributes.data(),
                .num_vertex_attributes =
                    static_cast<uint32_t>(info.vertex_attributes.size()),
            },
        .primitive_type = info.primitive_type,
        .rasterizer_state = {},
        .multisample_state = {},
        .depth_stencil_state = {},
        .target_info =
            SDL_GPUGraphicsPipelineTargetInfo{
                .color_target_descriptions = &color_target_description,
                .num_color_targets = 1,
                .depth_stencil_format = {},
                .has_depth_stencil_target = false,
                .padding1 = 0,
                .padding2 = 0,
                .padding3 = 0,
            },
        .props = 0,
    };

    auto* pipeline =
        SDL_CreateGPUGraphicsPipeline(this->device.get(), &create_info);

    if (pipeline == nullptr) {
        SDL_LogError(
            SDL_LOG_CATEGORY_ERROR,
            "Failed to create SDL_GPUGraphicsPipeline: %s",
            SDL_GetError()
        );
    }

    return std::unique_ptr<
        SDL_GPUGraphicsPipeline,
        GraphicsPipeline::SDL_GPUGraphicsPipelineDeleter>(
        pipeline,
        [this](SDL_GPUGraphicsPipeline* pipeline_to_release) {
            SDL_ReleaseGPUGraphicsPipeline(
                this->device.get(), pipeline_to_release
            );
        }
    );
}

auto GPUDevice::create_buffer(const BufferInfo& info) -> Buffer {
    const auto create_info = SDL_GPUBufferCreateInfo{
        .usage = static_cast<SDL_GPUBufferUsageFlags>(info.usage),
        .size = info.size,
        .props = 0,
    };

    auto* raw_buffer =
        SDL_CreateGPUBuffer(this->device.get(), &create_info);

    if (raw_buffer == nullptr) {
        SDL_LogError(
            SDL_LOG_CATEGORY_ERROR,
            "Failed to create SDL_GPUBuffer: %s",
            SDL_GetError()
        );
    }

    auto owned = std::unique_ptr<SDL_GPUBuffer, Buffer::SDL_GPUBufferDeleter>(
        raw_buffer,
        [this](SDL_GPUBuffer* buffer_to_release) {
            SDL_ReleaseGPUBuffer(this->device.get(), buffer_to_release);
        }
    );

    return Buffer{std::move(owned), info.size};
}

auto GPUDevice::create_transfer_buffer(const TransferBufferInfo& info)
    -> TransferBuffer {
    const auto create_info = SDL_GPUTransferBufferCreateInfo{
        .usage = static_cast<SDL_GPUTransferBufferUsage>(info.usage),
        .size = info.size,
        .props = 0,
    };

    auto* raw_transfer_buffer =
        SDL_CreateGPUTransferBuffer(this->device.get(), &create_info);

    if (raw_transfer_buffer == nullptr) {
        SDL_LogError(
            SDL_LOG_CATEGORY_ERROR,
            "Failed to create SDL_GPUTransferBuffer: %s",
            SDL_GetError()
        );
    }

    auto owned = std::unique_ptr<
        SDL_GPUTransferBuffer,
        TransferBuffer::SDL_GPUTransferBufferDeleter>(
        raw_transfer_buffer,
        [this](SDL_GPUTransferBuffer* transfer_buffer_to_release) {
            SDL_ReleaseGPUTransferBuffer(
                this->device.get(), transfer_buffer_to_release
            );
        }
    );

    return TransferBuffer{std::move(owned), this->device.get(), info.size};
}

}  // namespace Luminol::Graphics::SDL_GPU
