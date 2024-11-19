#include "SDL_GPUShader.hpp"

#include <fstream>

namespace {

using namespace Luminol::Graphics::SDL_GPU;

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

auto create_shader(GPUDevice& device, const ShaderInfo& info)
    -> SDL_GPUShader* {
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

    return SDL_CreateGPUShader(&device.get(), &sdl_gpu_shader_create_info);
}

}  // namespace

namespace Luminol::Graphics::SDL_GPU {

Shader::Shader(GPUDevice& device, const ShaderInfo& info)
    : shader{create_shader(device, info), [&device](SDL_GPUShader* shader) {
                 SDL_ReleaseGPUShader(&device.get(), shader);
             }} {
    Expects(this->shader);
}

}  // namespace Luminol::Graphics::SDL_GPU
