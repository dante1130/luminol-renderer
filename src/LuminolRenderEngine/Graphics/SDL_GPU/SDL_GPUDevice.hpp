#pragma once

#include <functional>
#include <memory>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTypes.hpp>

struct SDL_Window;
struct SDL_GPUDevice;
struct SDL_GPUFence;

namespace Luminol::Graphics::SDL_GPU {

class CommandBuffer;
class Shader;
struct ShaderInfo;
class GraphicsPipeline;
struct GraphicsPipelineInfo;
class ComputePipeline;
struct ComputePipelineInfo;
class Buffer;
struct BufferInfo;
class TransferBuffer;
struct TransferBufferInfo;
class Texture;
struct TextureInfo;
class Sampler;
struct SamplerInfo;

class GPUDevice : public std::enable_shared_from_this<GPUDevice> {
public:
    GPUDevice(SDL_Window* window);

    [[nodiscard]] auto native_handle() const -> SDL_GPUDevice*;

    [[nodiscard]] auto create_command_buffer() -> CommandBuffer;
    [[nodiscard]] auto create_shader(const ShaderInfo& info) -> Shader;
    [[nodiscard]] auto create_graphics_pipeline(const GraphicsPipelineInfo& info
    ) -> GraphicsPipeline;
    [[nodiscard]] auto create_compute_pipeline(const ComputePipelineInfo& info
    ) -> ComputePipeline;
    [[nodiscard]] auto create_buffer(const BufferInfo& info) -> Buffer;
    [[nodiscard]] auto create_transfer_buffer(const TransferBufferInfo& info)
        -> TransferBuffer;
    [[nodiscard]] auto create_texture(const TextureInfo& info) -> Texture;
    [[nodiscard]] auto create_sampler(const SamplerInfo& info) -> Sampler;

    [[nodiscard]] auto get_swapchain_texture_format(SDL_Window* window) const
        -> TextureFormat;

    [[nodiscard]] auto supports_sample_count(
        TextureFormat format, SampleCount sample_count
    ) const -> bool;

    // Blocks the calling thread until all submitted GPU work has completed.
    // Intended for readback/debug paths, not per-frame use.
    auto wait_for_idle() const -> void;

    // Best-effort debug/perf-testing toggle: Vsync is always supported, but
    // Immediate/Mailbox aren't guaranteed on every driver - returns false
    // (and logs, without asserting) if the requested mode isn't supported or
    // the change fails, leaving the swapchain on its previous mode. window
    // must already be claimed by this device.
    [[nodiscard]] auto set_present_mode(SDL_Window* window, PresentMode mode)
        const -> bool;

    // Blocks the calling thread until fence is signaled. fence must have come
    // from this device's CommandBuffer::submit_and_acquire_fence(); a null
    // fence is a no-op. Does not release the fence - call release_fence
    // afterward.
    auto wait_for_fence(SDL_GPUFence* fence) const -> void;

    // Releases a fence acquired from
    // CommandBuffer::submit_and_acquire_fence(). A null fence is a no-op.
    auto release_fence(SDL_GPUFence* fence) const -> void;

private:
    using SDL_GPUDeviceDeleter = std::function<void(SDL_GPUDevice*)>;

    std::unique_ptr<SDL_GPUDevice, SDL_GPUDeviceDeleter> device;
};

}  // namespace Luminol::Graphics::SDL_GPU
