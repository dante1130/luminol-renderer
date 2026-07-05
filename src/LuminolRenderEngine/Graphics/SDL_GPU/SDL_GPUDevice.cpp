#include "SDL_GPUDevice.hpp"

#include <fstream>
#include <stdexcept>
#include <vector>

#include <gsl/gsl>

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_log.h>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCommandBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUGraphicsPipeline.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUShader.hpp>
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

constexpr auto to_sdl_shader_stage(ShaderStage stage) -> SDL_GPUShaderStage {
    switch (stage) {
        case ShaderStage::Vertex:
            return SDL_GPU_SHADERSTAGE_VERTEX;
        case ShaderStage::Fragment:
            return SDL_GPU_SHADERSTAGE_FRAGMENT;
    }
    throw std::runtime_error{"Invalid shader stage"};
}

constexpr auto to_sdl_primitive_type(PrimitiveType type)
    -> SDL_GPUPrimitiveType {
    switch (type) {
        case PrimitiveType::TriangleList:
            return SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        case PrimitiveType::TriangleStrip:
            return SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP;
        case PrimitiveType::LineList:
            return SDL_GPU_PRIMITIVETYPE_LINELIST;
        case PrimitiveType::LineStrip:
            return SDL_GPU_PRIMITIVETYPE_LINESTRIP;
        case PrimitiveType::PointList:
            return SDL_GPU_PRIMITIVETYPE_POINTLIST;
    }
    return SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
}

constexpr auto to_sdl_texture_format(TextureFormat format)
    -> SDL_GPUTextureFormat {
    switch (format) {
        case TextureFormat::Invalid:
            return SDL_GPU_TEXTUREFORMAT_INVALID;
        case TextureFormat::B8G8R8A8_Unorm:
            return SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM;
        case TextureFormat::R8G8B8A8_Unorm:
            return SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    }
    return SDL_GPU_TEXTUREFORMAT_INVALID;
}

constexpr auto to_sdl_vertex_element_format(VertexElementFormat format)
    -> SDL_GPUVertexElementFormat {
    switch (format) {
        case VertexElementFormat::Float:
            return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT;
        case VertexElementFormat::Float2:
            return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
        case VertexElementFormat::Float3:
            return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
        case VertexElementFormat::Float4:
            return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
    }
    return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT;
}

constexpr auto to_sdl_vertex_input_rate(VertexInputRate rate)
    -> SDL_GPUVertexInputRate {
    switch (rate) {
        case VertexInputRate::Vertex:
            return SDL_GPU_VERTEXINPUTRATE_VERTEX;
        case VertexInputRate::Instance:
            return SDL_GPU_VERTEXINPUTRATE_INSTANCE;
    }
    return SDL_GPU_VERTEXINPUTRATE_VERTEX;
}

constexpr auto to_sdl_buffer_usage(BufferUsage usage)
    -> SDL_GPUBufferUsageFlags {
    switch (usage) {
        case BufferUsage::Vertex:
            return SDL_GPU_BUFFERUSAGE_VERTEX;
        case BufferUsage::Index:
            return SDL_GPU_BUFFERUSAGE_INDEX;
    }
    return SDL_GPU_BUFFERUSAGE_VERTEX;
}

constexpr auto to_sdl_transfer_buffer_usage(TransferBufferUsage usage)
    -> SDL_GPUTransferBufferUsage {
    switch (usage) {
        case TransferBufferUsage::Upload:
            return SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        case TransferBufferUsage::Download:
            return SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
    }
    return SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
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

auto GPUDevice::native_handle() const -> SDL_GPUDevice* {
    return device.get();
}

auto GPUDevice::create_command_buffer() -> CommandBuffer {
    auto* command_buffer = SDL_AcquireGPUCommandBuffer(device.get());

    if (command_buffer == nullptr) {
        SDL_LogError(
            SDL_LOG_CATEGORY_ERROR,
            "Failed to create SDL_GPUCommandBuffer: %s",
            SDL_GetError()
        );
    }

    return CommandBuffer{command_buffer};
}

auto GPUDevice::create_shader(const ShaderInfo& info) -> Shader {
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
        .stage = to_sdl_shader_stage(info.stage),
        .num_samplers = info.sampler_count,
        .num_storage_textures = info.storage_texture_count,
        .num_storage_buffers = info.storage_buffer_count,
        .num_uniform_buffers = info.uniform_buffer_count,
        .props = 0,
    };

    return Shader{std::unique_ptr<SDL_GPUShader, Shader::SDL_GPUShaderDeleter>(
        SDL_CreateGPUShader(this->device.get(), &sdl_gpu_shader_create_info),
        [this](SDL_GPUShader* shader) {
            SDL_ReleaseGPUShader(this->device.get(), shader);
        }
    )};
}

auto GPUDevice::create_graphics_pipeline(const GraphicsPipelineInfo& info)
    -> GraphicsPipeline {
    auto sdl_vertex_buffer_descriptions =
        std::vector<SDL_GPUVertexBufferDescription>{};
    sdl_vertex_buffer_descriptions.reserve(
        info.vertex_buffer_descriptions.size()
    );
    for (const auto& desc : info.vertex_buffer_descriptions) {
        sdl_vertex_buffer_descriptions.push_back(SDL_GPUVertexBufferDescription{
            .slot = desc.slot,
            .pitch = desc.pitch,
            .input_rate = to_sdl_vertex_input_rate(desc.input_rate),
            .instance_step_rate = desc.instance_step_rate,
        });
    }

    auto sdl_vertex_attributes = std::vector<SDL_GPUVertexAttribute>{};
    sdl_vertex_attributes.reserve(info.vertex_attributes.size());
    for (const auto& attr : info.vertex_attributes) {
        sdl_vertex_attributes.push_back(SDL_GPUVertexAttribute{
            .location = attr.location,
            .buffer_slot = attr.buffer_slot,
            .format = to_sdl_vertex_element_format(attr.format),
            .offset = attr.offset,
        });
    }

    const auto color_target_description = SDL_GPUColorTargetDescription{
        .format = to_sdl_texture_format(info.color_target_format),
        .blend_state = {},
    };

    const auto create_info = SDL_GPUGraphicsPipelineCreateInfo{
        .vertex_shader = info.vertex_shader.native_handle(),
        .fragment_shader = info.fragment_shader.native_handle(),
        .vertex_input_state =
            SDL_GPUVertexInputState{
                .vertex_buffer_descriptions =
                    sdl_vertex_buffer_descriptions.data(),
                .num_vertex_buffers = static_cast<uint32_t>(
                    sdl_vertex_buffer_descriptions.size()
                ),
                .vertex_attributes = sdl_vertex_attributes.data(),
                .num_vertex_attributes =
                    static_cast<uint32_t>(sdl_vertex_attributes.size()),
            },
        .primitive_type = to_sdl_primitive_type(info.primitive_type),
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

    return GraphicsPipeline{std::unique_ptr<
        SDL_GPUGraphicsPipeline,
        GraphicsPipeline::SDL_GPUGraphicsPipelineDeleter>(
        pipeline,
        [this](SDL_GPUGraphicsPipeline* pipeline_to_release) {
            SDL_ReleaseGPUGraphicsPipeline(
                this->device.get(), pipeline_to_release
            );
        }
    )};
}

auto GPUDevice::create_buffer(const BufferInfo& info) -> Buffer {
    const auto create_info = SDL_GPUBufferCreateInfo{
        .usage = to_sdl_buffer_usage(info.usage),
        .size = info.size,
        .props = 0,
    };

    auto* raw_buffer = SDL_CreateGPUBuffer(this->device.get(), &create_info);

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
        .usage = to_sdl_transfer_buffer_usage(info.usage),
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
