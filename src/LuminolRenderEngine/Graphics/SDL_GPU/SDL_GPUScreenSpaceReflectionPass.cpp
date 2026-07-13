#include "SDL_GPUScreenSpaceReflectionPass.hpp"

#include <array>
#include <cstddef>

#include <gsl/gsl>
#include <SDL3/SDL_video.h>

#include <LuminolMaths/Vector.hpp>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCommandBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUDevice.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPURenderPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUResourceBuilders.hpp>
#include <LuminolRenderEngine/Utilities/Timer.hpp>

namespace {

using namespace Luminol::Graphics::SDL_GPU;
using namespace Luminol::Maths;

// Matches the resolved HDR color format (hdr_color_texture) so the reflected
// radiance keeps full range; the alpha channel carries the hit confidence.
constexpr auto ssr_texture_format = TextureFormat::R16G16B16A16_Float;

// Mirrors cbuffer SSRBuffer in ssr_frag.hlsl.
struct SSRUniforms {
    Matrix4x4f projection_matrix;
    Matrix4x4f inverse_projection_matrix;
    Vector4f params;
    Vector4f viewport_size;
};

// Mirrors cbuffer ResolveBuffer in ssr_resolve_frag.hlsl.
struct ResolveUniforms {
    Vector4f viewport_size;
};

auto make_ssr_texture(GPUDevice& device, uint32_t width, uint32_t height)
    -> Texture {
    return device.create_texture(TextureInfo{
        .width = width,
        .height = height,
        .format = ssr_texture_format,
        .usage = TextureUsage::ColorTarget | TextureUsage::Sampler,
    });
}

auto make_ssr_texture(GPUDevice& device, SDL_Window* window) -> Texture {
    const auto [width, height] = get_window_size_in_pixels(window);
    return make_ssr_texture(device, width, height);
}

}  // namespace

namespace Luminol::Graphics::SDL_GPU {

SDL_GPUScreenSpaceReflectionPass::SDL_GPUScreenSpaceReflectionPass(
    GPUDevice& device, SDL_Window* window
)
    : fullscreen_vertex_shader{make_hlsl_shader(
          device, "res/shaders/sdl_gpu/fullscreen_vert.hlsl",
          ShaderStage::Vertex
      )},
      ssr_fragment_shader{make_hlsl_shader(
          device,
          "res/shaders/sdl_gpu/ssr_frag.hlsl",
          ShaderStage::Fragment,
          3U,
          1U
      )},
      ssr_pipeline{make_fullscreen_pipeline(
          device, fullscreen_vertex_shader, ssr_fragment_shader,
          ssr_texture_format
      )},
      resolve_fragment_shader{make_hlsl_shader(
          device,
          "res/shaders/sdl_gpu/ssr_resolve_frag.hlsl",
          ShaderStage::Fragment,
          1U,
          1U
      )},
      resolve_pipeline{make_fullscreen_pipeline(
          device, fullscreen_vertex_shader, resolve_fragment_shader,
          ssr_texture_format
      )},
      ssr_texture{make_ssr_texture(device, window)},
      ssr_resolved_texture{make_ssr_texture(device, window)},
      clamp_sampler{device.create_sampler(SamplerInfo{
          .filter = SamplerFilter::Linear,
          .address_mode_u = SamplerAddressMode::ClampToEdge,
          .address_mode_v = SamplerAddressMode::ClampToEdge,
      })} {}

auto SDL_GPUScreenSpaceReflectionPass::resize(
    GPUDevice& device, uint32_t width, uint32_t height
) -> void {
    ssr_texture = make_ssr_texture(device, width, height);
    ssr_resolved_texture = make_ssr_texture(device, width, height);
}

auto SDL_GPUScreenSpaceReflectionPass::draw(
    CommandBuffer& command_buffer,
    const Maths::Matrix4x4f& projection_matrix,
    const Texture& depth_texture,
    const Texture& normal_texture,
    const Texture& previous_color_texture,
    bool has_valid_previous_color,
    Utilities::PerformanceLogger& performance_logger
) -> void {
    const auto viewport_size = Vector4f{
        static_cast<float>(ssr_texture.get_width()),
        static_cast<float>(ssr_texture.get_height()),
        0.0F,
        0.0F,
    };

    // Trace pass: cast the reflection rays into ssr_texture.
    {
        const auto pass_timer = Utilities::Timer{};
        command_buffer.push_debug_group("ssr_trace");

        const auto ssr_texture_view = TextureView{ssr_texture.native_handle()};

        const auto color_targets = std::array{ColorTargetInfo{
            .texture = &ssr_texture_view,
            .clear_color = {0.0F, 0.0F, 0.0F, 0.0F},
            .load_op = LoadOp::Clear,
            .store_op = StoreOp::Store,
            .cycle = true,
        }};

        auto render_pass = command_buffer.begin_render_pass(color_targets);
        render_pass.bind_graphics_pipeline(ssr_pipeline);

        const auto ssr_uniforms = SSRUniforms{
            .projection_matrix = projection_matrix,
            .inverse_projection_matrix = projection_matrix.inverse(),
            .params = Vector4f{
                max_distance,
                thickness,
                max_steps,
                has_valid_previous_color ? 1.0F : 0.0F,
            },
            .viewport_size = viewport_size,
        };
        command_buffer.push_fragment_uniform_data(
            0,
            gsl::span{
                reinterpret_cast<const std::byte*>(&ssr_uniforms),
                sizeof(ssr_uniforms)
            }
        );

        const auto sampler_bindings = std::array{
            TextureSamplerBinding{
                .texture = &depth_texture, .sampler = &clamp_sampler
            },
            TextureSamplerBinding{
                .texture = &normal_texture, .sampler = &clamp_sampler
            },
            TextureSamplerBinding{
                .texture = &previous_color_texture, .sampler = &clamp_sampler
            },
        };
        render_pass.bind_fragment_samplers(0, sampler_bindings);

        render_pass.draw_primitives(3, 1, 0, 0);

        command_buffer.pop_debug_group();
        performance_logger.record(
            "ssr_trace", Units::Seconds{pass_timer.elapsed_seconds()}
        );
    }

    // Resolve pass: confidence-weighted blur into ssr_resolved_texture,
    // denoising the jittered trace result.
    {
        const auto pass_timer = Utilities::Timer{};
        command_buffer.push_debug_group("ssr_resolve");

        const auto resolved_texture_view =
            TextureView{ssr_resolved_texture.native_handle()};

        const auto color_targets = std::array{ColorTargetInfo{
            .texture = &resolved_texture_view,
            .clear_color = {0.0F, 0.0F, 0.0F, 0.0F},
            .load_op = LoadOp::Clear,
            .store_op = StoreOp::Store,
            .cycle = true,
        }};

        auto render_pass = command_buffer.begin_render_pass(color_targets);
        render_pass.bind_graphics_pipeline(resolve_pipeline);

        const auto resolve_uniforms = ResolveUniforms{
            .viewport_size = viewport_size,
        };
        command_buffer.push_fragment_uniform_data(
            0,
            gsl::span{
                reinterpret_cast<const std::byte*>(&resolve_uniforms),
                sizeof(resolve_uniforms)
            }
        );

        const auto sampler_bindings = std::array{TextureSamplerBinding{
            .texture = &ssr_texture, .sampler = &clamp_sampler
        }};
        render_pass.bind_fragment_samplers(0, sampler_bindings);

        render_pass.draw_primitives(3, 1, 0, 0);

        command_buffer.pop_debug_group();
        performance_logger.record(
            "ssr_resolve", Units::Seconds{pass_timer.elapsed_seconds()}
        );
    }
}

auto SDL_GPUScreenSpaceReflectionPass::get_ssr_texture() const
    -> const Texture& {
    return ssr_resolved_texture;
}

auto SDL_GPUScreenSpaceReflectionPass::get_sampler() const -> const Sampler& {
    return clamp_sampler;
}

}  // namespace Luminol::Graphics::SDL_GPU
