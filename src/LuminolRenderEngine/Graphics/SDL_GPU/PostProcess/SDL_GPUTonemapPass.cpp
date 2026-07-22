#include "SDL_GPUTonemapPass.hpp"

#include <array>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCommandBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUDevice.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/RenderPasses/SDL_GPURenderPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUResourceBuilders.hpp>

namespace {

using namespace Luminol::Graphics::SDL_GPU;

struct TonemapUniforms {
    float exposure;
};

}  // namespace

namespace Luminol::Graphics::SDL_GPU {

SDL_GPUTonemapPass::SDL_GPUTonemapPass(GPUDevice& device, SDL_Window* window)
    : fullscreen_vertex_shader{make_hlsl_shader(
          device, "res/shaders/sdl_gpu/fullscreen_vert.hlsl",
          ShaderStage::Vertex
      )},
      tonemap_fragment_shader{make_hlsl_shader(
          device,
          "res/shaders/sdl_gpu/tonemap_frag.hlsl",
          ShaderStage::Fragment,
          1U,
          1U
      )},
      tonemap_pipeline{make_fullscreen_pipeline(
          device, fullscreen_vertex_shader, tonemap_fragment_shader,
          device.get_swapchain_texture_format(window)
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
