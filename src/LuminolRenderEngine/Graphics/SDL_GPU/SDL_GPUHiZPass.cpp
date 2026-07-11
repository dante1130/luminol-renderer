#include "SDL_GPUHiZPass.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <filesystem>

#include <gsl/gsl>
#include <SDL3/SDL_video.h>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCommandBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUComputePass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUDevice.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPURenderPass.hpp>

namespace {

using namespace Luminol::Graphics::SDL_GPU;

constexpr auto pyramid_format = TextureFormat::R32_Float;
constexpr auto downsample_threads = uint32_t{8};

auto compute_mip_levels(uint32_t width, uint32_t height) -> uint32_t {
    return 1U +
        static_cast<uint32_t>(std::floor(
            std::log2(static_cast<float>(std::max(width, height)))
        ));
}

// Mirrors cbuffer HiZDownsampleParams in hiz_downsample.hlsl.
struct HiZDownsampleParams {
    std::array<uint32_t, 2> dst_size;
    std::array<uint32_t, 2> src_size;
};

auto make_pyramid_texture(GPUDevice& device, uint32_t width, uint32_t height)
    -> Texture {
    return device.create_texture(TextureInfo{
        .width = width,
        .height = height,
        .format = pyramid_format,
        .usage = TextureUsage::ColorTarget | TextureUsage::ComputeStorageRead |
            TextureUsage::ComputeStorageWrite | TextureUsage::Sampler,
        .mip_levels = compute_mip_levels(width, height),
    });
}

auto make_pyramid_texture(GPUDevice& device, SDL_Window* window) -> Texture {
    auto width = int{0};
    auto height = int{0};
    SDL_GetWindowSizeInPixels(window, &width, &height);

    return make_pyramid_texture(
        device, static_cast<uint32_t>(width), static_cast<uint32_t>(height)
    );
}

auto make_shader(
    GPUDevice& device, const std::filesystem::path& path, ShaderStage stage,
    uint32_t sampler_count
) -> Shader {
    return device.create_shader(ShaderInfo{
        .path = path,
        .stage = stage,
        .source_language = ShaderSourceLanguage::Hlsl,
        .sampler_count = sampler_count,
    });
}

auto make_copy_depth_pipeline(
    GPUDevice& device, const Shader& vertex_shader, const Shader& fragment_shader
) -> GraphicsPipeline {
    return device.create_graphics_pipeline(GraphicsPipelineInfo{
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_target_format = pyramid_format,
        .primitive_type = PrimitiveType::TriangleList,
        .vertex_buffer_descriptions = {},
        .vertex_attributes = {},
        .enable_depth_test = false,
        .cull_mode = CullMode::None,
    });
}

auto make_downsample_pipeline(GPUDevice& device) -> ComputePipeline {
    return device.create_compute_pipeline(ComputePipelineInfo{
        .path = "res/shaders/sdl_gpu/hiz_downsample.hlsl",
        .source_language = ShaderSourceLanguage::Hlsl,
        .readwrite_storage_texture_count = 2,
        .uniform_buffer_count = 1,
        .threadcount_x = downsample_threads,
        .threadcount_y = downsample_threads,
        .threadcount_z = 1,
    });
}

auto make_pyramid_sampler(GPUDevice& device, uint32_t mip_levels) -> Sampler {
    return device.create_sampler(SamplerInfo{
        .filter = SamplerFilter::Nearest,
        .address_mode_u = SamplerAddressMode::ClampToEdge,
        .address_mode_v = SamplerAddressMode::ClampToEdge,
        .max_lod = mip_levels - 1,
    });
}

}  // namespace

namespace Luminol::Graphics::SDL_GPU {

SDL_GPUHiZPass::SDL_GPUHiZPass(GPUDevice& device, SDL_Window* window)
    : fullscreen_vertex_shader{make_shader(
          device, "res/shaders/sdl_gpu/fullscreen_vert.hlsl",
          ShaderStage::Vertex, 0U
      )},
      copy_depth_fragment_shader{make_shader(
          device, "res/shaders/sdl_gpu/hiz_copy_depth.hlsl",
          ShaderStage::Fragment, 1U
      )},
      copy_depth_pipeline{make_copy_depth_pipeline(
          device, fullscreen_vertex_shader, copy_depth_fragment_shader
      )},
      downsample_pipeline{make_downsample_pipeline(device)},
      pyramid_texture{make_pyramid_texture(device, window)},
      pyramid_sampler{
          make_pyramid_sampler(device, pyramid_texture.get_mip_levels())
      } {}

auto SDL_GPUHiZPass::resize(GPUDevice& device, uint32_t width, uint32_t height)
    -> void {
    pyramid_texture = make_pyramid_texture(device, width, height);
    pyramid_sampler =
        make_pyramid_sampler(device, pyramid_texture.get_mip_levels());
}

auto SDL_GPUHiZPass::build(
    CommandBuffer& command_buffer,
    const Texture& previous_frame_depth_texture,
    const Sampler& point_sampler
) -> void {
    const auto pyramid_view = TextureView{pyramid_texture.native_handle()};
    const auto mip_levels = pyramid_texture.get_mip_levels();

    // Mip 0: copy raw device depth from last frame's depth texture via a
    // fullscreen triangle. cycle=true since last frame's cull compute pass
    // may still have an in-flight read of this same texture.
    {
        const auto color_targets = std::array{ColorTargetInfo{
            .texture = &pyramid_view,
            .clear_color = {1.0F, 1.0F, 1.0F, 1.0F},
            .load_op = LoadOp::Clear,
            .store_op = StoreOp::Store,
            .cycle = true,
            .mip_level = 0,
        }};

        auto render_pass = command_buffer.begin_render_pass(color_targets);
        render_pass.bind_graphics_pipeline(copy_depth_pipeline);

        const auto sampler_bindings = std::array{TextureSamplerBinding{
            .texture = &previous_frame_depth_texture, .sampler = &point_sampler
        }};
        render_pass.bind_fragment_samplers(0, sampler_bindings);

        render_pass.draw_primitives(3, 1, 0, 0);
    }

    // Mips 1..N-1: max-reduction compute downsample. One compute pass per
    // mip transition - SDL_GPU doesn't synchronize texture reads/writes
    // across dispatches within a single compute pass, so each mip's write
    // must be visible before the next mip reads it.
    auto src_width = pyramid_texture.get_width();
    auto src_height = pyramid_texture.get_height();
    for (auto mip = uint32_t{1}; mip < mip_levels; ++mip) {
        const auto dst_width = std::max(src_width / 2, 1U);
        const auto dst_height = std::max(src_height / 2, 1U);

        const auto storage_texture_bindings = std::array{
            StorageTextureReadWriteBinding{
                .texture = &pyramid_texture, .mip_level = mip - 1, .cycle = false
            },
            StorageTextureReadWriteBinding{
                .texture = &pyramid_texture, .mip_level = mip, .cycle = false
            },
        };
        auto compute_pass =
            command_buffer.begin_compute_pass(storage_texture_bindings, {});
        compute_pass.bind_compute_pipeline(downsample_pipeline);

        const auto params = HiZDownsampleParams{
            .dst_size = {dst_width, dst_height},
            .src_size = {src_width, src_height},
        };
        command_buffer.push_compute_uniform_data(
            0,
            gsl::span<const std::byte>{
                reinterpret_cast<const std::byte*>(&params), sizeof(params)
            }
        );

        const auto group_count_x =
            (dst_width + downsample_threads - 1) / downsample_threads;
        const auto group_count_y =
            (dst_height + downsample_threads - 1) / downsample_threads;
        compute_pass.dispatch(group_count_x, group_count_y, 1);

        src_width = dst_width;
        src_height = dst_height;
    }
}

auto SDL_GPUHiZPass::get_pyramid_texture() const -> const Texture& {
    return pyramid_texture;
}

auto SDL_GPUHiZPass::get_pyramid_sampler() const -> const Sampler& {
    return pyramid_sampler;
}

auto SDL_GPUHiZPass::get_mip_levels() const -> uint32_t {
    return pyramid_texture.get_mip_levels();
}

}  // namespace Luminol::Graphics::SDL_GPU
