#pragma once

#include <functional>
#include <memory>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTypes.hpp>

struct SDL_Window;
struct SDL_GPUDevice;

namespace Luminol::Graphics::SDL_GPU {

class CommandBuffer;
class Shader;
struct ShaderInfo;
class GraphicsPipeline;
struct GraphicsPipelineInfo;
class Buffer;
struct BufferInfo;
class TransferBuffer;
struct TransferBufferInfo;

class GPUDevice {
public:
    GPUDevice(SDL_Window* window);

    [[nodiscard]] auto native_handle() const -> SDL_GPUDevice*;

    [[nodiscard]] auto create_command_buffer() -> CommandBuffer;
    [[nodiscard]] auto create_shader(const ShaderInfo& info) -> Shader;
    [[nodiscard]] auto create_graphics_pipeline(const GraphicsPipelineInfo& info
    ) -> GraphicsPipeline;
    [[nodiscard]] auto create_buffer(const BufferInfo& info) -> Buffer;
    [[nodiscard]] auto create_transfer_buffer(const TransferBufferInfo& info)
        -> TransferBuffer;

    [[nodiscard]] auto get_swapchain_texture_format(SDL_Window* window) const
        -> TextureFormat;

private:
    using SDL_GPUDeviceDeleter = std::function<void(SDL_GPUDevice*)>;

    std::unique_ptr<SDL_GPUDevice, SDL_GPUDeviceDeleter> device;
};

}  // namespace Luminol::Graphics::SDL_GPU
