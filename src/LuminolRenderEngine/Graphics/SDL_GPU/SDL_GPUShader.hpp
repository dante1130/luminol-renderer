#pragma once

#include <filesystem>
#include <functional>
#include <memory>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUDevice.hpp>

namespace Luminol::Graphics::SDL_GPU {

enum class ShaderStage : uint8_t {
    Vertex,
    Fragment,
};

struct ShaderInfo {
    std::filesystem::path path;
    ShaderStage stage;
    uint32_t sampler_count = {0};
    uint32_t uniform_buffer_count = {0};
    uint32_t storage_buffer_count = {0};
    uint32_t storage_texture_count = {0};
};

class Shader {
public:
    using SDL_GPUShaderDeleter = std::function<void(SDL_GPUShader*)>;

    Shader(std::unique_ptr<SDL_GPUShader, SDL_GPUShaderDeleter> shader);

private:
    std::unique_ptr<SDL_GPUShader, SDL_GPUShaderDeleter> shader;
};

}  // namespace Luminol::Graphics::SDL_GPU
