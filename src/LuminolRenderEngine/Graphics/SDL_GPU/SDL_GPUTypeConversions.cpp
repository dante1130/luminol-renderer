#include "SDL_GPUTypeConversions.hpp"

#include <stdexcept>

namespace Luminol::Graphics::SDL_GPU {

auto to_sdl_shader_stage(ShaderStage stage) -> SDL_GPUShaderStage {
    switch (stage) {
        case ShaderStage::Vertex:
            return SDL_GPU_SHADERSTAGE_VERTEX;
        case ShaderStage::Fragment:
            return SDL_GPU_SHADERSTAGE_FRAGMENT;
        case ShaderStage::Compute:
            break;
    }
    throw std::runtime_error{"Invalid shader stage"};
}

auto to_shadercross_stage(ShaderStage stage)
    -> SDL_ShaderCross_ShaderStage {
    switch (stage) {
        case ShaderStage::Vertex:
            return SDL_SHADERCROSS_SHADERSTAGE_VERTEX;
        case ShaderStage::Fragment:
            return SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT;
        case ShaderStage::Compute:
            return SDL_SHADERCROSS_SHADERSTAGE_COMPUTE;
    }
    throw std::runtime_error{"Invalid shader stage"};
}

auto from_sdl_texture_format(SDL_GPUTextureFormat format)
    -> TextureFormat {
    switch (format) {
        case SDL_GPU_TEXTUREFORMAT_INVALID:
            return TextureFormat::Invalid;
        case SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM:
            return TextureFormat::B8G8R8A8_Unorm;
        case SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM:
            return TextureFormat::R8G8B8A8_Unorm;
        case SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM_SRGB:
            return TextureFormat::R8G8B8A8_Unorm_Srgb;
        case SDL_GPU_TEXTUREFORMAT_D24_UNORM:
            return TextureFormat::D24_Unorm;
        case SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT:
            return TextureFormat::R16G16B16A16_Float;
        default:
            return TextureFormat::Invalid;
    }
}

auto to_sdl_primitive_type(PrimitiveType type)
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
    throw std::runtime_error{"Invalid primitive type"};
}

auto to_sdl_cull_mode(CullMode mode) -> SDL_GPUCullMode {
    switch (mode) {
        case CullMode::None:
            return SDL_GPU_CULLMODE_NONE;
        case CullMode::Front:
            return SDL_GPU_CULLMODE_FRONT;
        case CullMode::Back:
            return SDL_GPU_CULLMODE_BACK;
    }
    throw std::runtime_error{"Invalid cull mode"};
}

auto to_sdl_front_face(FrontFace front_face) -> SDL_GPUFrontFace {
    switch (front_face) {
        case FrontFace::CounterClockwise:
            return SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
        case FrontFace::Clockwise:
            return SDL_GPU_FRONTFACE_CLOCKWISE;
    }
    throw std::runtime_error{"Invalid front face"};
}

auto to_sdl_texture_format(TextureFormat format)
    -> SDL_GPUTextureFormat {
    switch (format) {
        case TextureFormat::Invalid:
            return SDL_GPU_TEXTUREFORMAT_INVALID;
        case TextureFormat::B8G8R8A8_Unorm:
            return SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM;
        case TextureFormat::R8G8B8A8_Unorm:
            return SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        case TextureFormat::R8G8B8A8_Unorm_Srgb:
            return SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM_SRGB;
        case TextureFormat::D24_Unorm:
            return SDL_GPU_TEXTUREFORMAT_D24_UNORM;
        case TextureFormat::R16G16B16A16_Float:
            return SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT;
    }
    throw std::runtime_error{"Invalid texture format"};
}

auto to_sdl_texture_usage(TextureUsage usage) -> SDL_GPUTextureUsageFlags {
    auto flags = SDL_GPUTextureUsageFlags{0};

    if ((usage & TextureUsage::Sampler) == TextureUsage::Sampler) {
        flags |= SDL_GPU_TEXTUREUSAGE_SAMPLER;
    }
    if ((usage & TextureUsage::ColorTarget) == TextureUsage::ColorTarget) {
        flags |= SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
    }
    if ((usage & TextureUsage::DepthStencilTarget) ==
        TextureUsage::DepthStencilTarget) {
        flags |= SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
    }

    return flags;
}

auto to_sdl_sample_count(SampleCount sample_count) -> SDL_GPUSampleCount {
    switch (sample_count) {
        case SampleCount::x1:
            return SDL_GPU_SAMPLECOUNT_1;
        case SampleCount::x2:
            return SDL_GPU_SAMPLECOUNT_2;
        case SampleCount::x4:
            return SDL_GPU_SAMPLECOUNT_4;
        case SampleCount::x8:
            return SDL_GPU_SAMPLECOUNT_8;
    }
    throw std::runtime_error{"Invalid sample count"};
}

auto to_sdl_vertex_element_format(VertexElementFormat format)
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
    throw std::runtime_error{"Invalid vertex element format"};
}

auto to_sdl_vertex_input_rate(VertexInputRate rate)
    -> SDL_GPUVertexInputRate {
    switch (rate) {
        case VertexInputRate::Vertex:
            return SDL_GPU_VERTEXINPUTRATE_VERTEX;
        case VertexInputRate::Instance:
            return SDL_GPU_VERTEXINPUTRATE_INSTANCE;
    }
    throw std::runtime_error{"Invalid vertex input rate"};
}

auto to_sdl_buffer_usage(BufferUsage usage)
    -> SDL_GPUBufferUsageFlags {
    switch (usage) {
        case BufferUsage::Vertex:
            return SDL_GPU_BUFFERUSAGE_VERTEX;
        case BufferUsage::Index:
            return SDL_GPU_BUFFERUSAGE_INDEX;
        case BufferUsage::StorageRead:
            return SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ;
        case BufferUsage::ComputeStorageRead:
            return SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ;
        case BufferUsage::ComputeStorageReadWrite:
            return SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE;
    }
    throw std::runtime_error{"Invalid buffer usage"};
}

auto to_sdl_transfer_buffer_usage(TransferBufferUsage usage)
    -> SDL_GPUTransferBufferUsage {
    switch (usage) {
        case TransferBufferUsage::Upload:
            return SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        case TransferBufferUsage::Download:
            return SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
    }
    throw std::runtime_error{"Invalid transfer buffer usage"};
}

auto to_sdl_filter(SamplerFilter filter) -> SDL_GPUFilter {
    switch (filter) {
        case SamplerFilter::Nearest:
            return SDL_GPU_FILTER_NEAREST;
        case SamplerFilter::Linear:
            return SDL_GPU_FILTER_LINEAR;
    }
    throw std::runtime_error{"Invalid sampler filter"};
}

auto to_sdl_sampler_address_mode(SamplerAddressMode address_mode)
    -> SDL_GPUSamplerAddressMode {
    switch (address_mode) {
        case SamplerAddressMode::Repeat:
            return SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
        case SamplerAddressMode::ClampToEdge:
            return SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        case SamplerAddressMode::MirroredRepeat:
            return SDL_GPU_SAMPLERADDRESSMODE_MIRRORED_REPEAT;
    }
    throw std::runtime_error{"Invalid sampler address mode"};
}

}  // namespace Luminol::Graphics::SDL_GPU
