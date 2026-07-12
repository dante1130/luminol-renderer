#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>

#include <SDL3/SDL_video.h>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCommandBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUComputePass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUComputePipeline.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCopyPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUDevice.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTransferBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTypes.hpp>
#include <LuminolRenderEngine/Window/Window.hpp>

// Validates the compute pipeline plumbing (HLSL -> SPIR-V -> compute
// pipeline -> dispatch -> readback) added for Clustered Forward+. Dispatches
// compute_smoke_test.hlsl, which writes each thread's dispatch index into a
// read-write storage buffer, and asserts the readback matches.

namespace {

constexpr auto thread_count = uint32_t{256};
constexpr auto threads_per_group = uint32_t{64};

}  // namespace

auto main() -> int {
    using namespace Luminol;
    using namespace Luminol::Graphics::SDL_GPU;

    auto window = Window{1, 1, "Luminol Compute Smoke Test"};
    auto gpu_device = std::make_shared<GPUDevice>(
        static_cast<SDL_Window*>(window.get_window_handle())
    );

    auto pipeline = gpu_device->create_compute_pipeline(ComputePipelineInfo{
        .path = "res/shaders/sdl_gpu/compute_smoke_test.hlsl",
        .source_language = ShaderSourceLanguage::Hlsl,
        .readwrite_storage_buffer_count = 1,
        .threadcount_x = threads_per_group,
        .threadcount_y = 1,
        .threadcount_z = 1,
    });

    constexpr auto buffer_size =
        thread_count * static_cast<uint32_t>(sizeof(uint32_t));

    auto output_buffer = gpu_device->create_buffer(BufferInfo{
        .usage = BufferUsage::ComputeStorageReadWrite,
        .size = buffer_size,
    });

    auto download_transfer_buffer =
        gpu_device->create_transfer_buffer(TransferBufferInfo{
            .usage = TransferBufferUsage::Download,
            .size = buffer_size,
        });

    auto command_buffer = gpu_device->create_command_buffer();
    {
        const auto storage_bindings =
            std::array<StorageBufferReadWriteBinding, 1>{
                StorageBufferReadWriteBinding{
                    .buffer = &output_buffer, .cycle = false
                }
            };
        auto compute_pass = command_buffer.begin_compute_pass({}, storage_bindings);
        compute_pass.bind_compute_pipeline(pipeline);
        compute_pass.dispatch(thread_count / threads_per_group, 1, 1);
    }
    {
        auto copy_pass = command_buffer.begin_copy_pass();
        copy_pass.download_from_buffer(
            output_buffer, 0, download_transfer_buffer, 0, buffer_size
        );
    }
    command_buffer.submit();
    gpu_device->wait_for_idle();

    const auto mapped = download_transfer_buffer.map(false);
    auto results = std::vector<uint32_t>(thread_count);
    std::memcpy(results.data(), mapped.data(), buffer_size);
    download_transfer_buffer.unmap();

    auto success = true;
    for (auto i = uint32_t{0}; i < thread_count; ++i) {
        if (results[i] != i) {
            std::printf(
                "Compute smoke test FAILED at index %u: expected %u, got %u\n",
                i,
                i,
                results[i]
            );
            success = false;
        }
    }

    if (success) {
        std::printf(
            "Compute smoke test PASSED (%u threads verified)\n", thread_count
        );
    }

    return success ? 0 : 1;
}
