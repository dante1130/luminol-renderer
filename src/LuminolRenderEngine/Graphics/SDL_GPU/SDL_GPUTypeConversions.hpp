#pragma once

#include <SDL3/SDL_gpu.h>
#include <SDL3_shadercross/SDL_shadercross.h>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTypes.hpp>

namespace Luminol::Graphics::SDL_GPU {

[[nodiscard]] auto to_sdl_shader_stage(ShaderStage stage)
    -> SDL_GPUShaderStage;

[[nodiscard]] auto to_shadercross_stage(ShaderStage stage)
    -> SDL_ShaderCross_ShaderStage;

[[nodiscard]] auto from_sdl_texture_format(SDL_GPUTextureFormat format)
    -> TextureFormat;

[[nodiscard]] auto to_sdl_primitive_type(PrimitiveType type)
    -> SDL_GPUPrimitiveType;

[[nodiscard]] auto to_sdl_texture_format(TextureFormat format)
    -> SDL_GPUTextureFormat;

[[nodiscard]] auto to_sdl_vertex_element_format(VertexElementFormat format)
    -> SDL_GPUVertexElementFormat;

[[nodiscard]] auto to_sdl_vertex_input_rate(VertexInputRate rate)
    -> SDL_GPUVertexInputRate;

[[nodiscard]] auto to_sdl_buffer_usage(BufferUsage usage)
    -> SDL_GPUBufferUsageFlags;

[[nodiscard]] auto to_sdl_transfer_buffer_usage(TransferBufferUsage usage)
    -> SDL_GPUTransferBufferUsage;

[[nodiscard]] auto to_sdl_filter(SamplerFilter filter) -> SDL_GPUFilter;

[[nodiscard]] auto to_sdl_sampler_address_mode(SamplerAddressMode address_mode)
    -> SDL_GPUSamplerAddressMode;

}  // namespace Luminol::Graphics::SDL_GPU
