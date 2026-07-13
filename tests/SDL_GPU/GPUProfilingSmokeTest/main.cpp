#include <cstdio>
#include <memory>

#include <SDL3/SDL_video.h>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCommandBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUDevice.hpp>
#include <LuminolRenderEngine/Window/Window.hpp>

// Validates that the GPU debug-group/label annotations and the fence-based
// submit/wait/release path (added for opt-in GPU performance profiling) run
// without crashing or logging an SDL error. This does not (and cannot,
// without a native-backend interop SDL_GPU doesn't expose) verify actual GPU
// timing values - see the doc comments on CommandBuffer::submit_and_acquire_fence
// and PerformanceLogger::log_and_reset for why.

auto main() -> int {
    using namespace Luminol;
    using namespace Luminol::Graphics::SDL_GPU;

    auto window = Window{1, 1, "Luminol GPU Profiling Smoke Test"};
    auto gpu_device = std::make_shared<GPUDevice>(
        static_cast<SDL_Window*>(window.get_window_handle())
    );

    {
        auto command_buffer = gpu_device->create_command_buffer();

        command_buffer.push_debug_group("smoke_test_group");
        command_buffer.insert_debug_label("smoke_test_label");
        command_buffer.pop_debug_group();

        command_buffer.submit();
    }

    {
        auto command_buffer = gpu_device->create_command_buffer();

        command_buffer.push_debug_group("smoke_test_fence_group");
        command_buffer.pop_debug_group();
        auto* fence = command_buffer.submit_and_acquire_fence();

        gpu_device->wait_for_fence(fence);
        gpu_device->release_fence(fence);
    }

    // A null fence must also be a safe no-op.
    gpu_device->wait_for_fence(nullptr);
    gpu_device->release_fence(nullptr);

    std::printf("GPU profiling smoke test PASSED\n");
    return 0;
}
