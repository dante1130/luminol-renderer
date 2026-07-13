#include "SDL_GPUResourceBuilders.hpp"

#include <optional>

#include <SDL3/SDL_video.h>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUDevice.hpp>

namespace Luminol::Graphics::SDL_GPU {

auto get_window_size_in_pixels(SDL_Window* window)
    -> std::pair<uint32_t, uint32_t> {
    auto width = int{0};
    auto height = int{0};
    SDL_GetWindowSizeInPixels(window, &width, &height);

    return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
}

auto make_hlsl_shader(
    GPUDevice& device,
    const std::filesystem::path& path,
    ShaderStage stage,
    uint32_t sampler_count,
    uint32_t uniform_buffer_count,
    uint32_t storage_buffer_count,
    uint32_t storage_texture_count
) -> Shader {
    return device.create_shader(ShaderInfo{
        .path = path,
        .stage = stage,
        .source_language = ShaderSourceLanguage::Hlsl,
        .sampler_count = sampler_count,
        .uniform_buffer_count = uniform_buffer_count,
        .storage_buffer_count = storage_buffer_count,
        .storage_texture_count = storage_texture_count,
    });
}

auto make_fullscreen_pipeline(
    GPUDevice& device,
    const Shader& vertex_shader,
    const Shader& fragment_shader,
    TextureFormat color_target_format
) -> GraphicsPipeline {
    return device.create_graphics_pipeline(GraphicsPipelineInfo{
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_target_format = color_target_format,
        .primitive_type = PrimitiveType::TriangleList,
        .vertex_buffer_descriptions = {},
        .vertex_attributes = {},
        .enable_depth_test = false,
        .cull_mode = CullMode::None,
    });
}

auto make_depth_only_mesh_pipeline(
    GPUDevice& device,
    const Shader& vertex_shader,
    const Shader& fragment_shader,
    TextureFormat depth_stencil_format,
    SampleCount sample_count
) -> GraphicsPipeline {
    return device.create_graphics_pipeline(GraphicsPipelineInfo{
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_target_format = std::nullopt,
        .primitive_type = PrimitiveType::TriangleList,
        .vertex_buffer_descriptions = mesh_vertex_buffer_descriptions,
        .vertex_attributes = mesh_vertex_attributes,
        .enable_depth_test = true,
        .depth_stencil_format = depth_stencil_format,
        .cull_mode = CullMode::Back,
        .front_face = FrontFace::Clockwise,
        .sample_count = sample_count,
    });
}

}  // namespace Luminol::Graphics::SDL_GPU
