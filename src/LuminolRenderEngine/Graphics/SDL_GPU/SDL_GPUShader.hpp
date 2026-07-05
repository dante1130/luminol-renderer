#pragma once

#include <filesystem>
#include <functional>
#include <memory>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTypes.hpp>

struct SDL_GPUShader;

namespace Luminol::Graphics::SDL_GPU {

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

    [[nodiscard]] auto native_handle() const -> SDL_GPUShader*;

private:
    std::unique_ptr<SDL_GPUShader, SDL_GPUShaderDeleter> shader;
};

}  // namespace Luminol::Graphics::SDL_GPU
