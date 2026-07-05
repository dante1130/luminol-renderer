#pragma once

#include <functional>
#include <memory>

#include <gsl/gsl>
#include <SDL3/SDL_gpu.h>

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

    [[nodiscard]] auto get() const -> const SDL_GPUDevice&;
    [[nodiscard]] auto get() -> SDL_GPUDevice&;

    [[nodiscard]] auto create_command_buffer() -> CommandBuffer;
    [[nodiscard]] auto create_shader(const ShaderInfo& info) -> Shader;
    [[nodiscard]] auto create_graphics_pipeline(const GraphicsPipelineInfo& info
    ) -> GraphicsPipeline;
    [[nodiscard]] auto create_buffer(const BufferInfo& info) -> Buffer;
    [[nodiscard]] auto create_transfer_buffer(const TransferBufferInfo& info)
        -> TransferBuffer;
private:
    using SDL_GPUDeviceDeleter = std::function<void(SDL_GPUDevice*)>;

    std::unique_ptr<SDL_GPUDevice, SDL_GPUDeviceDeleter> device;
};

}  // namespace Luminol::Graphics::SDL_GPU
