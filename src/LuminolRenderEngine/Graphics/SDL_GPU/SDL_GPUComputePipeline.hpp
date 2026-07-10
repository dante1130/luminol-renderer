#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include <string>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTypes.hpp>

struct SDL_GPUComputePipeline;

namespace Luminol::Graphics::SDL_GPU {

// Unlike graphics shaders (Shader + GraphicsPipeline as two objects),
// SDL_GPU compiles and links a compute shader directly into a single
// pipeline object, so there is no separate ComputeShader type.
struct ComputePipelineInfo {
    std::filesystem::path path;
    ShaderSourceLanguage source_language = ShaderSourceLanguage::SpirvBinary;
    std::string entrypoint = "main";
    uint32_t sampler_count = {0};
    uint32_t readonly_storage_texture_count = {0};
    uint32_t readonly_storage_buffer_count = {0};
    uint32_t readwrite_storage_texture_count = {0};
    uint32_t readwrite_storage_buffer_count = {0};
    uint32_t uniform_buffer_count = {0};
    uint32_t threadcount_x = {1};
    uint32_t threadcount_y = {1};
    uint32_t threadcount_z = {1};
};

class ComputePipeline {
public:
    using SDL_GPUComputePipelineDeleter =
        std::function<void(SDL_GPUComputePipeline*)>;

    ComputePipeline(std::unique_ptr<
                     SDL_GPUComputePipeline,
                     SDL_GPUComputePipelineDeleter> pipeline);

    [[nodiscard]] auto native_handle() const -> SDL_GPUComputePipeline*;

private:
    std::unique_ptr<SDL_GPUComputePipeline, SDL_GPUComputePipelineDeleter>
        pipeline;
};

}  // namespace Luminol::Graphics::SDL_GPU
