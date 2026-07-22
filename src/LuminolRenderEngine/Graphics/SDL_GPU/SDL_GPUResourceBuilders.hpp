#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <utility>

#include <gsl/gsl>

#include <LuminolRenderEngine/Graphics/SDL_GPU/RenderPasses/SDL_GPUCopyPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUGraphicsPipeline.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUShader.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTexture.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTransferBuffer.hpp>
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

// Clamp-to-edge, linear-filtered sampler shape shared by most fullscreen
// passes. enable_compare has no default: it must be passed explicitly
// because SDL silently accepts a sampler with the wrong compare setting
// (no compile error), which would break shadow PCF sampling.
[[nodiscard]] auto make_clamp_linear_sampler(
    GPUDevice& device,
    bool enable_compare,
    bool enable_mipmap_filtering = false
) -> Sampler;

// Maps transfer_buffer, copies data into it, unmaps, then uploads it into
// buffer via copy_pass. Caller owns sizing/growing both buffers beforehand -
// this only performs the CPU-to-GPU staging copy.
auto upload_via_transfer(
    CopyPass& copy_pass,
    TransferBuffer& transfer_buffer,
    const Buffer& buffer,
    gsl::span<const std::byte> data
) -> void;

}  // namespace Luminol::Graphics::SDL_GPU
