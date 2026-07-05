#pragma once

#include <optional>

#include <gsl/gsl>
#include <SDL3/SDL_gpu.h>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPURenderPass.hpp>

namespace Luminol::Graphics::SDL_GPU {

struct SwapchainTexture {
    SDL_GPUTexture* texture;
    uint32_t width;
    uint32_t height;
};

struct ColorTargetInfo {
    SDL_GPUTexture* texture = nullptr;
    SDL_FColor clear_color = {};
    SDL_GPULoadOp load_op = SDL_GPU_LOADOP_CLEAR;
    SDL_GPUStoreOp store_op = SDL_GPU_STOREOP_STORE;
};

class CommandBuffer {
public:
    CommandBuffer(SDL_GPUCommandBuffer* command_buffer);
    ~CommandBuffer();

    CommandBuffer(const CommandBuffer&) = delete;
    auto operator=(const CommandBuffer&) -> CommandBuffer& = delete;

    CommandBuffer(CommandBuffer&& other) noexcept;
    auto operator=(CommandBuffer&& other) noexcept -> CommandBuffer&;

    [[nodiscard]] auto acquire_swapchain_texture(SDL_Window* window)
        -> std::optional<SwapchainTexture>;

    [[nodiscard]] auto begin_render_pass(
        gsl::span<const ColorTargetInfo> color_targets
    ) -> RenderPass;

    auto submit() -> void;
    auto cancel() -> void;

private:
    SDL_GPUCommandBuffer* command_buffer;
};

}  // namespace Luminol::Graphics::SDL_GPU
