#include "SDL_GPUTonemapPass.hpp"

#include <array>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCommandBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUDevice.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPURenderPass.hpp>

namespace {

using namespace Luminol::Graphics::SDL_GPU;

struct TonemapUniforms {
    float exposure;
};

auto make_shader(
    GPUDevice& device,
    const std::filesystem::path& path,
    ShaderStage stage,
    uint32_t sampler_count,
    uint32_t uniform_buffer_count
) -> Shader {
    return device.create_shader(ShaderInfo{
        .path = path,
        .stage = stage,
        .source_language = ShaderSourceLanguage::Hlsl,
        .sampler_count = sampler_count,
        .uniform_buffer_count = uniform_buffer_count,
        .storage_buffer_count = 0U,
    });
}

auto make_tonemap_pipeline(
    GPUDevice& device,
    SDL_Window* window,
    const Shader& vertex_shader,
    const Shader& fragment_shader
) -> GraphicsPipeline {
    return device.create_graphics_pipeline(GraphicsPipelineInfo{
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_target_format = device.get_swapchain_texture_format(window),
        .primitive_type = PrimitiveType::TriangleList,
        .vertex_buffer_descriptions = {},
        .vertex_attributes = {},
        .enable_depth_test = false,
        .cull_mode = CullMode::None,
    });
}

}  // namespace

namespace Luminol::Graphics::SDL_GPU {

SDL_GPUTonemapPass::SDL_GPUTonemapPass(GPUDevice& device, SDL_Window* window)
    : fullscreen_vertex_shader{make_shader(
          device,
          "res/shaders/sdl_gpu/fullscreen_vert.hlsl",
          ShaderStage::Vertex,
          0U,
          0U
      )},
      tonemap_fragment_shader{make_shader(
          device,
          "res/shaders/sdl_gpu/tonemap_frag.hlsl",
          ShaderStage::Fragment,
          1U,
          1U
      )},
      tonemap_pipeline{make_tonemap_pipeline(
          device, window, fullscreen_vertex_shader, tonemap_fragment_shader
      )},
      clamp_sampler{device.create_sampler(SamplerInfo{
          .filter = SamplerFilter::Linear,
          .address_mode_u = SamplerAddressMode::ClampToEdge,
          .address_mode_v = SamplerAddressMode::ClampToEdge,
      })} {}

auto SDL_GPUTonemapPass::draw(
    CommandBuffer& command_buffer,
    RenderPass& render_pass,
    const Texture& hdr_color_texture,
    float exposure
) const -> void {
    render_pass.bind_graphics_pipeline(tonemap_pipeline);

    const auto tonemap_uniforms = TonemapUniforms{.exposure = exposure};
    command_buffer.push_fragment_uniform_data(
        0,
        gsl::span{
            reinterpret_cast<const std::byte*>(&tonemap_uniforms),
            sizeof(tonemap_uniforms)
        }
    );

    const auto sampler_bindings = std::array{TextureSamplerBinding{
        .texture = &hdr_color_texture, .sampler = &clamp_sampler
    }};
    render_pass.bind_fragment_samplers(0, sampler_bindings);

    render_pass.draw_primitives(3, 1, 0, 0);
}

}  // namespace Luminol::Graphics::SDL_GPU
