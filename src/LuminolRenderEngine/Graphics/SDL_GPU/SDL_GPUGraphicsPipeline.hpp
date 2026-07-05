#pragma once

#include <functional>
#include <memory>

#include <gsl/gsl>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUShader.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTypes.hpp>

struct SDL_GPUGraphicsPipeline;

namespace Luminol::Graphics::SDL_GPU {

struct GraphicsPipelineInfo {
    const Shader& vertex_shader;
    const Shader& fragment_shader;
    TextureFormat color_target_format;
    PrimitiveType primitive_type = PrimitiveType::TriangleList;
    gsl::span<const VertexBufferDescription> vertex_buffer_descriptions;
    gsl::span<const VertexAttribute> vertex_attributes;
};

class GraphicsPipeline {
public:
    using SDL_GPUGraphicsPipelineDeleter =
        std::function<void(SDL_GPUGraphicsPipeline*)>;

    GraphicsPipeline(std::unique_ptr<
                     SDL_GPUGraphicsPipeline,
                     SDL_GPUGraphicsPipelineDeleter> pipeline);

    [[nodiscard]] auto native_handle() const -> SDL_GPUGraphicsPipeline*;

private:
    std::unique_ptr<SDL_GPUGraphicsPipeline, SDL_GPUGraphicsPipelineDeleter>
        pipeline;
};

}  // namespace Luminol::Graphics::SDL_GPU
