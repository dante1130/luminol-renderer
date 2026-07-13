#include "SDL_GPUHiZPass.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

#include <gsl/gsl>
#include <SDL3/SDL_video.h>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCommandBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUComputePass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUDevice.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPURenderPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUResourceBuilders.hpp>

namespace {

using namespace Luminol::Graphics::SDL_GPU;

constexpr auto pyramid_format = TextureFormat::R32_Float;
constexpr auto downsample_threads = uint32_t{8};

// One 8x8 threadgroup can reduce its own 64 computed values through
// groupshared memory down to 4x4, 2x2, and 1x1 - 3 extra mip levels beyond
// the one it computes directly from src_mip, so up to 4 mip transitions fit
// in a single dispatch. See hiz_downsample.hlsl.
constexpr auto max_mips_per_dispatch = uint32_t{4};

auto compute_mip_levels(uint32_t width, uint32_t height) -> uint32_t {
    return 1U +
        static_cast<uint32_t>(std::floor(
            std::log2(static_cast<float>(std::max(width, height)))
        ));
}

// Mirrors cbuffer HiZDownsampleParams in hiz_downsample.hlsl.
struct HiZDownsampleParams {
    std::array<uint32_t, 2> src_size;
    std::array<uint32_t, 2> dst_size0;
    std::array<uint32_t, 2> dst_size1;
    std::array<uint32_t, 2> dst_size2;
    std::array<uint32_t, 2> dst_size3;
    uint32_t num_mips_this_dispatch;
    std::array<uint32_t, 3> padding;
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
    const auto [width, height] = get_window_size_in_pixels(window);
    return make_pyramid_texture(device, width, height);
}

auto make_downsample_pipeline(GPUDevice& device) -> ComputePipeline {
    return device.create_compute_pipeline(ComputePipelineInfo{
        .path = "res/shaders/sdl_gpu/hiz_downsample.hlsl",
        .source_language = ShaderSourceLanguage::Hlsl,
        .readwrite_storage_texture_count = 5,
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
    : fullscreen_vertex_shader{make_hlsl_shader(
          device, "res/shaders/sdl_gpu/fullscreen_vert.hlsl",
          ShaderStage::Vertex
      )},
      copy_depth_fragment_shader{make_hlsl_shader(
          device, "res/shaders/sdl_gpu/hiz_copy_depth.hlsl",
          ShaderStage::Fragment, 1U
      )},
      copy_depth_pipeline{make_fullscreen_pipeline(
          device, fullscreen_vertex_shader, copy_depth_fragment_shader,
          pyramid_format
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

    // Mips 1..N-1: max-reduction compute downsample, batched up to
    // max_mips_per_dispatch transitions per dispatch (see the doc comment on
    // hiz_downsample.hlsl for how one threadgroup produces multiple mips via
    // groupshared memory). Each batch still depends on the previous batch's
    // last mip being fully written first - SDL_GPU doesn't synchronize
    // texture reads/writes across separate begin_compute_pass calls, so
    // batches remain sequential; only the mips *within* one batch collapse
    // into a single dispatch.
    auto src_width = pyramid_texture.get_width();
    auto src_height = pyramid_texture.get_height();
    for (auto mip = uint32_t{1}; mip < mip_levels;) {
        const auto mips_this_batch =
            std::min(max_mips_per_dispatch, mip_levels - mip);

        auto dst_widths = std::array<uint32_t, max_mips_per_dispatch>{};
        auto dst_heights = std::array<uint32_t, max_mips_per_dispatch>{};
        {
            auto w = src_width;
            auto h = src_height;
            for (auto i = uint32_t{0}; i < mips_this_batch; ++i) {
                w = std::max(w / 2, 1U);
                h = std::max(h / 2, 1U);
                dst_widths[i] = w;
                dst_heights[i] = h;
            }
        }

        // Pipeline always declares 5 storage textures (1 src + 4 dst) -
        // pad any unused dst slots (mips_this_batch < 4, only possible on
        // the final batch) by reusing the last real mip's view. The shader
        // never writes through them in that case (guarded by
        // num_mips_this_dispatch), so the aliasing is harmless.
        const auto last_real_mip = mip + mips_this_batch - 1;
        const auto storage_texture_bindings = std::array{
            StorageTextureReadWriteBinding{
                .texture = &pyramid_texture, .mip_level = mip - 1, .cycle = false
            },
            StorageTextureReadWriteBinding{
                .texture = &pyramid_texture, .mip_level = mip, .cycle = false
            },
            StorageTextureReadWriteBinding{
                .texture = &pyramid_texture,
                .mip_level = mips_this_batch > 1 ? mip + 1 : last_real_mip,
                .cycle = false,
            },
            StorageTextureReadWriteBinding{
                .texture = &pyramid_texture,
                .mip_level = mips_this_batch > 2 ? mip + 2 : last_real_mip,
                .cycle = false,
            },
            StorageTextureReadWriteBinding{
                .texture = &pyramid_texture,
                .mip_level = mips_this_batch > 3 ? mip + 3 : last_real_mip,
                .cycle = false,
            },
        };
        auto compute_pass =
            command_buffer.begin_compute_pass(storage_texture_bindings, {});
        compute_pass.bind_compute_pipeline(downsample_pipeline);

        const auto params = HiZDownsampleParams{
            .src_size = {src_width, src_height},
            .dst_size0 = {dst_widths[0], dst_heights[0]},
            .dst_size1 = {dst_widths[1], dst_heights[1]},
            .dst_size2 = {dst_widths[2], dst_heights[2]},
            .dst_size3 = {dst_widths[3], dst_heights[3]},
            .num_mips_this_dispatch = mips_this_batch,
            .padding = {0, 0, 0},
        };
        command_buffer.push_compute_uniform_data(
            0,
            gsl::span<const std::byte>{
                reinterpret_cast<const std::byte*>(&params), sizeof(params)
            }
        );

        // Dispatch is sized to the batch's first (finest) output mip - the
        // one every thread computes directly; later, coarser mips in the
        // same batch are produced by a subset of those same threads via
        // groupshared reduction, not extra threads.
        const auto group_count_x =
            (dst_widths[0] + downsample_threads - 1) / downsample_threads;
        const auto group_count_y =
            (dst_heights[0] + downsample_threads - 1) / downsample_threads;
        compute_pass.dispatch(group_count_x, group_count_y, 1);

        src_width = dst_widths[mips_this_batch - 1];
        src_height = dst_heights[mips_this_batch - 1];
        mip += mips_this_batch;
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
