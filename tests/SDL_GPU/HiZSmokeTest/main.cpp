#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
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
// reduction) in plain C++ from a known 8x8 input, and comparing against the
// GPU-produced pyramid read back per-mip. hiz_copy_depth.hlsl (mip 0) is a
// plain Texture2D.Sample of an ordinary sampled texture, not an actual
// depth-stencil resource, so the input can be a small R32_Float texture with
// hand-picked values - no depth-format handling needed.

namespace {

using namespace Luminol::Graphics::SDL_GPU;

constexpr auto pyramid_size = uint32_t{8};
constexpr auto mip_count = uint32_t{4};  // 8x8, 4x4, 2x2, 1x1
constexpr auto epsilon = 0.001F;

auto approx_equal(float lhs, float rhs) -> bool {
    return std::abs(lhs - rhs) <= epsilon;
}

// One 2x2 max-reduction step, mirroring hiz_downsample.hlsl exactly (all
// dimensions here are even, so no odd-dimension corner clamping is needed).
auto downsample(
    const std::vector<float>& src, uint32_t src_width, uint32_t src_height
) -> std::vector<float> {
    const auto dst_width = src_width / 2;
    const auto dst_height = src_height / 2;
    auto dst = std::vector<float>(static_cast<size_t>(dst_width) * dst_height);

    for (auto y = uint32_t{0}; y < dst_height; ++y) {
        for (auto x = uint32_t{0}; x < dst_width; ++x) {
            const auto x0 = x * 2;
            const auto y0 = y * 2;
            const auto v0 = src[(y0 * src_width) + x0];
            const auto v1 = src[(y0 * src_width) + x0 + 1];
            const auto v2 = src[((y0 + 1) * src_width) + x0];
            const auto v3 = src[((y0 + 1) * src_width) + x0 + 1];
            dst[(y * dst_width) + x] =
                std::max(std::max(v0, v1), std::max(v2, v3));
        }
    }

    return dst;
}

}  // namespace

auto main() -> int {
    using namespace Luminol;
    using namespace Luminol::Graphics::SDL_GPU;

    auto window = Window{1, 1, "Luminol HiZ Smoke Test"};
    auto gpu_device = std::make_shared<GPUDevice>(
        static_cast<SDL_Window*>(window.get_window_handle())
    );

    auto hiz_pass = SDL_GPUHiZPass{
        *gpu_device, static_cast<SDL_Window*>(window.get_window_handle())
    };
    hiz_pass.resize(*gpu_device, pyramid_size, pyramid_size);

    auto depth_texture = gpu_device->create_texture(TextureInfo{
        .width = pyramid_size,
        .height = pyramid_size,
        .format = TextureFormat::R32_Float,
        .usage = TextureUsage::Sampler,
        .mip_levels = 1,
    });
    auto point_sampler = gpu_device->create_sampler(SamplerInfo{
        .filter = SamplerFilter::Nearest,
        .address_mode_u = SamplerAddressMode::ClampToEdge,
        .address_mode_v = SamplerAddressMode::ClampToEdge,
        .max_lod = 0,
    });

    auto mip0_expected = std::vector<float>(pyramid_size * pyramid_size);
    for (auto y = uint32_t{0}; y < pyramid_size; ++y) {
        for (auto x = uint32_t{0}; x < pyramid_size; ++x) {
            mip0_expected[(y * pyramid_size) + x] =
                static_cast<float>(x) + (static_cast<float>(y) * pyramid_size);
        }
    }

    constexpr auto mip0_byte_size =
        uint32_t{pyramid_size * pyramid_size * sizeof(float)};

    auto upload_buffer = gpu_device->create_transfer_buffer(TransferBufferInfo{
        .usage = TransferBufferUsage::Upload,
        .size = mip0_byte_size,
    });

    auto command_buffer = gpu_device->create_command_buffer();
    {
        auto mapped = upload_buffer.map(true);
        std::memcpy(mapped.data(), mip0_expected.data(), mip0_byte_size);
        upload_buffer.unmap();

        auto copy_pass = command_buffer.begin_copy_pass();
        copy_pass.upload_to_texture(
            upload_buffer, 0, depth_texture, pyramid_size, pyramid_size, true
        );
    }

    hiz_pass.build(command_buffer, depth_texture, point_sampler);

    auto download_buffers = std::vector<TransferBuffer>{};
    download_buffers.reserve(mip_count);
    for (auto mip = uint32_t{0}; mip < mip_count; ++mip) {
        const auto mip_size = pyramid_size >> mip;
        download_buffers.push_back(
            gpu_device->create_transfer_buffer(TransferBufferInfo{
                .usage = TransferBufferUsage::Download,
                .size = mip_size * mip_size * static_cast<uint32_t>(sizeof(float)),
            })
        );
    }
    {
        auto copy_pass = command_buffer.begin_copy_pass();
        for (auto mip = uint32_t{0}; mip < mip_count; ++mip) {
            const auto mip_size = pyramid_size >> mip;
            copy_pass.download_from_texture(
                hiz_pass.get_pyramid_texture(),
                mip,
                mip_size,
                mip_size,
                download_buffers[mip],
                0
            );
        }
    }
    command_buffer.submit();
    gpu_device->wait_for_idle();

    auto expected_mips = std::vector<std::vector<float>>{mip0_expected};
    for (auto mip = uint32_t{1}; mip < mip_count; ++mip) {
        const auto src_size = pyramid_size >> (mip - 1);
        expected_mips.push_back(
            downsample(expected_mips.back(), src_size, src_size)
        );
    }

    auto success = true;
    for (auto mip = uint32_t{0}; mip < mip_count; ++mip) {
        const auto mip_size = pyramid_size >> mip;
        const auto element_count = static_cast<size_t>(mip_size) * mip_size;

        auto actual = std::vector<float>(element_count);
        {
            const auto mapped = download_buffers[mip].map(false);
            std::memcpy(
                actual.data(),
                mapped.data(),
                element_count * sizeof(float)
            );
            download_buffers[mip].unmap();
        }

        const auto& expected = expected_mips[mip];
        for (auto i = size_t{0}; i < element_count; ++i) {
            if (!approx_equal(actual[i], expected[i])) {
                std::printf(
                    "HiZ smoke test FAILED at mip %u, index %zu: expected "
                    "%f, got %f\n",
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
            "HiZ smoke test PASSED (%u mip levels verified)\n", mip_count
        );
    }

    return success ? 0 : 1;
}
