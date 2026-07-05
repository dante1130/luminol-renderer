#pragma once

#include <functional>
#include <memory>

#include <gsl/gsl>
#include <SDL3/SDL_gpu.h>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUShader.hpp>

namespace Luminol::Graphics::SDL_GPU {

struct GraphicsPipelineInfo {
    const Shader& vertex_shader;
    const Shader& fragment_shader;
    SDL_GPUTextureFormat color_target_format;
    SDL_GPUPrimitiveType primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    gsl::span<const SDL_GPUVertexBufferDescription> vertex_buffer_descriptions;
    gsl::span<const SDL_GPUVertexAttribute> vertex_attributes;
};

class GraphicsPipeline {
public:
    using SDL_GPUGraphicsPipelineDeleter =
        std::function<void(SDL_GPUGraphicsPipeline*)>;

    GraphicsPipeline(std::unique_ptr<
                     SDL_GPUGraphicsPipeline,
                     SDL_GPUGraphicsPipelineDeleter> pipeline);

    [[nodiscard]] auto get() const -> SDL_GPUGraphicsPipeline*;

private:
    std::unique_ptr<SDL_GPUGraphicsPipeline, SDL_GPUGraphicsPipelineDeleter>
        pipeline;
};

}  // namespace Luminol::Graphics::SDL_GPU
