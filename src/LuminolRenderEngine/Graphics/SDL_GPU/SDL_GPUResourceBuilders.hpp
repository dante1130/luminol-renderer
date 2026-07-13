#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <utility>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUGraphicsPipeline.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUShader.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTypes.hpp>

struct SDL_Window;

namespace Luminol::Graphics::SDL_GPU {

class GPUDevice;

// Shared vertex layout for the standard PBR mesh vertex format (position,
// uv, normal, tangent) used by every pass that draws mesh geometry directly.
constexpr auto mesh_vertex_stride_in_floats = 11U;
constexpr auto mesh_vertex_stride_in_bytes =
    sizeof(float) * mesh_vertex_stride_in_floats;

constexpr auto mesh_vertex_buffer_descriptions = std::array{
    VertexBufferDescription{
        .slot = 0,
        .pitch = mesh_vertex_stride_in_bytes,
    },
};

constexpr auto mesh_vertex_attributes = std::array{
    VertexAttribute{
        .location = 0,
        .buffer_slot = 0,
        .format = VertexElementFormat::Float3,
        .offset = 0,
    },
    VertexAttribute{
        .location = 1,
        .buffer_slot = 0,
        .format = VertexElementFormat::Float2,
        .offset = sizeof(float) * 3,
    },
    VertexAttribute{
        .location = 2,
        .buffer_slot = 0,
        .format = VertexElementFormat::Float3,
        .offset = sizeof(float) * 5,
    },
    VertexAttribute{
        .location = 3,
        .buffer_slot = 0,
        .format = VertexElementFormat::Float3,
        .offset = sizeof(float) * 8,
    },
};

[[nodiscard]] auto get_window_size_in_pixels(SDL_Window* window)
    -> std::pair<uint32_t, uint32_t>;

[[nodiscard]] auto make_hlsl_shader(
    GPUDevice& device,
    const std::filesystem::path& path,
    ShaderStage stage,
    uint32_t sampler_count = 0,
    uint32_t uniform_buffer_count = 0,
    uint32_t storage_buffer_count = 0,
    uint32_t storage_texture_count = 0
) -> Shader;

// Fullscreen-triangle pipeline shape: no vertex input, depth test disabled,
// no culling, single color target.
[[nodiscard]] auto make_fullscreen_pipeline(
    GPUDevice& device,
    const Shader& vertex_shader,
    const Shader& fragment_shader,
    TextureFormat color_target_format
) -> GraphicsPipeline;

// Depth-only mesh pipeline shape (e.g. shadow maps): standard mesh vertex
// layout, no color target, depth test enabled, back-face culling.
// sample_count defaults to x1 (every current caller is a single-sample
// depth/shadow map); pass the real value to build one matching an MSAA
// color target's sample count instead (e.g. a depth pre-pass sharing the
// main pass's MSAA depth buffer).
[[nodiscard]] auto make_depth_only_mesh_pipeline(
    GPUDevice& device,
    const Shader& vertex_shader,
    const Shader& fragment_shader,
    TextureFormat depth_stencil_format,
    SampleCount sample_count = SampleCount::x1
) -> GraphicsPipeline;

}  // namespace Luminol::Graphics::SDL_GPU
