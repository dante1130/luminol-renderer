#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include <SDL3/SDL_video.h>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCommandBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCopyPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUDevice.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUHiZPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTexture.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTransferBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTypes.hpp>
#include <LuminolRenderEngine/Window/Window.hpp>

// Validates SDL_GPUHiZPass::build by independently recomputing the expected
// max-reduction mip pyramid (mirroring hiz_downsample.hlsl's 2x2 max
// reduction, including its corner-clamping for odd/1-sized dimensions) in
// plain C++ from a known input, and comparing against the GPU-produced
// pyramid read back per-mip. hiz_copy_depth.hlsl (mip 0) is a plain
// Texture2D.Sample of an ordinary sampled texture, not an actual
// depth-stencil resource, so the input can be a small R32_Float texture with
// hand-picked values - no depth-format handling needed.
//
// Two cases run: an 8x8 square (every dimension halves cleanly, matching the
// hiz_downsample.hlsl rewrite's simplest path with no clamping and exactly 4
// mips - one full dispatch batch), and a 64x5 non-square case (width is a
// clean power of 2, but height hits 1 at mip level 2 and then has to
// "self-reference" for 4 more transitions while width keeps halving - this
// is the corner-clamping path the 8x8 case cannot exercise, is the common
// case for any real, non-1:1 window aspect ratio, and spans 2 dispatch
// batches (4 transitions + 2), covering both intra-dispatch
// groupshared-memory clamping and clamping across a batch boundary).

namespace {

using namespace Luminol::Graphics::SDL_GPU;

constexpr auto epsilon = 0.001F;

auto approx_equal(float lhs, float rhs) -> bool {
    return std::abs(lhs - rhs) <= epsilon;
}

auto compute_mip_levels(uint32_t width, uint32_t height) -> uint32_t {
    return 1U +
        static_cast<uint32_t>(
            std::floor(std::log2(static_cast<float>(std::max(width, height))))
        );
}

// One 2x2 max-reduction step, mirroring hiz_downsample.hlsl exactly:
// footprint corners are clamped to the source's last valid texel in each
// axis independently, so odd (or already-1) source dimensions duplicate
// their edge instead of reading past it.
auto downsample(
    const std::vector<float>& src, uint32_t src_width, uint32_t src_height
) -> std::vector<float> {
    const auto dst_width = std::max(src_width / 2U, 1U);
    const auto dst_height = std::max(src_height / 2U, 1U);
    auto dst = std::vector<float>(static_cast<size_t>(dst_width) * dst_height);

    const auto src_max_x = src_width - 1U;
    const auto src_max_y = src_height - 1U;

    for (auto y = uint32_t{0}; y < dst_height; ++y) {
        for (auto x = uint32_t{0}; x < dst_width; ++x) {
            const auto base_x = x * 2U;
            const auto base_y = y * 2U;
            const auto x0 = std::min(base_x, src_max_x);
            const auto x1 = std::min(base_x + 1U, src_max_x);
            const auto y0 = std::min(base_y, src_max_y);
            const auto y1 = std::min(base_y + 1U, src_max_y);

            const auto v0 = src[(y0 * src_width) + x0];
            const auto v1 = src[(y0 * src_width) + x1];
            const auto v2 = src[(y1 * src_width) + x0];
            const auto v3 = src[(y1 * src_width) + x1];
            dst[(y * dst_width) + x] =
                std::max(std::max(v0, v1), std::max(v2, v3));
        }
    }

    return dst;
}

auto run_test(
    GPUDevice& gpu_device,
    SDL_Window* window,
    uint32_t width,
    uint32_t height,
    const std::string& case_name
) -> bool {
    const auto mip_count = compute_mip_levels(width, height);

    auto hiz_pass = SDL_GPUHiZPass{gpu_device, window};
    hiz_pass.resize(gpu_device, width, height);

    auto depth_texture = gpu_device.create_texture(TextureInfo{
        .width = width,
        .height = height,
        .format = TextureFormat::R32_Float,
        .usage = TextureUsage::Sampler,
        .mip_levels = 1,
    });
    auto point_sampler = gpu_device.create_sampler(SamplerInfo{
        .filter = SamplerFilter::Nearest,
        .address_mode_u = SamplerAddressMode::ClampToEdge,
        .address_mode_v = SamplerAddressMode::ClampToEdge,
        .max_lod = 0,
    });

    auto mip0_expected = std::vector<float>(static_cast<size_t>(width) * height);
    for (auto y = uint32_t{0}; y < height; ++y) {
        for (auto x = uint32_t{0}; x < width; ++x) {
            mip0_expected[(y * width) + x] =
                static_cast<float>(x) + (static_cast<float>(y) * width);
        }
    }

    const auto mip0_byte_size =
        static_cast<uint32_t>(width) * height * static_cast<uint32_t>(sizeof(float));

    auto upload_buffer = gpu_device.create_transfer_buffer(TransferBufferInfo{
        .usage = TransferBufferUsage::Upload,
        .size = mip0_byte_size,
    });

    auto command_buffer = gpu_device.create_command_buffer();
    {
        auto mapped = upload_buffer.map(true);
        std::memcpy(mapped.data(), mip0_expected.data(), mip0_byte_size);
        upload_buffer.unmap();

        auto copy_pass = command_buffer.begin_copy_pass();
        copy_pass.upload_to_texture(
            upload_buffer, 0, depth_texture, width, height, true
        );
    }

    hiz_pass.build(command_buffer, depth_texture, point_sampler);

    auto mip_widths = std::vector<uint32_t>{width};
    auto mip_heights = std::vector<uint32_t>{height};
    for (auto mip = uint32_t{1}; mip < mip_count; ++mip) {
        mip_widths.push_back(std::max(mip_widths.back() / 2U, 1U));
        mip_heights.push_back(std::max(mip_heights.back() / 2U, 1U));
    }

    auto download_buffers = std::vector<TransferBuffer>{};
    download_buffers.reserve(mip_count);
    for (auto mip = uint32_t{0}; mip < mip_count; ++mip) {
        download_buffers.push_back(
            gpu_device.create_transfer_buffer(TransferBufferInfo{
                .usage = TransferBufferUsage::Download,
                .size = mip_widths[mip] * mip_heights[mip] *
                    static_cast<uint32_t>(sizeof(float)),
            })
        );
    }
    {
        auto copy_pass = command_buffer.begin_copy_pass();
        for (auto mip = uint32_t{0}; mip < mip_count; ++mip) {
            copy_pass.download_from_texture(
                hiz_pass.get_pyramid_texture(),
                mip,
                mip_widths[mip],
                mip_heights[mip],
                download_buffers[mip],
                0
            );
        }
    }
    command_buffer.submit();
    gpu_device.wait_for_idle();

    auto expected_mips = std::vector<std::vector<float>>{mip0_expected};
    for (auto mip = uint32_t{1}; mip < mip_count; ++mip) {
        expected_mips.push_back(downsample(
            expected_mips.back(), mip_widths[mip - 1], mip_heights[mip - 1]
        ));
    }

    auto success = true;
    for (auto mip = uint32_t{0}; mip < mip_count; ++mip) {
        const auto element_count =
            static_cast<size_t>(mip_widths[mip]) * mip_heights[mip];

        auto actual = std::vector<float>(element_count);
        {
            const auto mapped = download_buffers[mip].map(false);
            std::memcpy(
                actual.data(), mapped.data(), element_count * sizeof(float)
            );
            download_buffers[mip].unmap();
        }

        const auto& expected = expected_mips[mip];
        for (auto i = size_t{0}; i < element_count; ++i) {
            if (!approx_equal(actual[i], expected[i])) {
                std::printf(
                    "HiZ smoke test [%s] FAILED at mip %u, index %zu: "
                    "expected %f, got %f\n",
                    case_name.c_str(),
                    mip,
                    i,
                    expected[i],
                    actual[i]
                );
                success = false;
            }
        }
    }

    if (success) {
        std::printf(
            "HiZ smoke test [%s] PASSED (%u mip levels verified)\n",
            case_name.c_str(),
            mip_count
        );
    }

    return success;
}

}  // namespace

auto main() -> int {
    using namespace Luminol;
    using namespace Luminol::Graphics::SDL_GPU;

    auto window = Window{1, 1, "Luminol HiZ Smoke Test"};
    auto gpu_device = std::make_shared<GPUDevice>(
        static_cast<SDL_Window*>(window.get_window_handle())
    );

    auto success = true;
    success &= run_test(
        *gpu_device,
        static_cast<SDL_Window*>(window.get_window_handle()),
        8,
        8,
        "8x8 square"
    );
    success &= run_test(
        *gpu_device,
        static_cast<SDL_Window*>(window.get_window_handle()),
        64,
        5,
        "64x5 non-square"
    );

    return success ? 0 : 1;
}
