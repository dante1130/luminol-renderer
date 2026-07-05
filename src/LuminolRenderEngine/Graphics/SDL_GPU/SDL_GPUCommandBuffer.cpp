#include "SDL_GPUCommandBuffer.hpp"

#include <vector>

#include <gsl/gsl>

#include <SDL3/SDL_log.h>

namespace Luminol::Graphics::SDL_GPU {

CommandBuffer::CommandBuffer(SDL_GPUCommandBuffer* command_buffer)
    : command_buffer{command_buffer} {
    Expects(this->command_buffer != nullptr);
}

CommandBuffer::~CommandBuffer() {
    if (command_buffer != nullptr) {
        SDL_CancelGPUCommandBuffer(command_buffer);
    }
}

CommandBuffer::CommandBuffer(CommandBuffer&& other) noexcept
    : command_buffer{other.command_buffer} {
    other.command_buffer = nullptr;
}

auto CommandBuffer::operator=(CommandBuffer&& other) noexcept -> CommandBuffer& {
    if (this != &other) {
        if (command_buffer != nullptr) {
            SDL_CancelGPUCommandBuffer(command_buffer);
        }
        command_buffer = other.command_buffer;
        other.command_buffer = nullptr;
    }
    return *this;
}

auto CommandBuffer::acquire_swapchain_texture(SDL_Window* window)
    -> std::optional<SwapchainTexture> {
    Expects(command_buffer != nullptr);

    SDL_GPUTexture* texture = nullptr;
    uint32_t width = 0;
    uint32_t height = 0;

    if (!SDL_WaitAndAcquireGPUSwapchainTexture(
            command_buffer, window, &texture, &width, &height
        )) {
        SDL_LogError(
            SDL_LOG_CATEGORY_ERROR,
            "Failed to acquire GPU swapchain texture: %s",
            SDL_GetError()
        );
        return std::nullopt;
    }

    if (texture == nullptr) {
        return std::nullopt;
    }

    return SwapchainTexture{
        .texture = texture, .width = width, .height = height
    };
}

auto CommandBuffer::begin_render_pass(
    gsl::span<const ColorTargetInfo> color_targets
) -> RenderPass {
    Expects(command_buffer != nullptr);

    auto sdl_color_targets =
        std::vector<SDL_GPUColorTargetInfo>(color_targets.size());
    for (auto i = size_t{0}; i < color_targets.size(); ++i) {
        sdl_color_targets[i] = SDL_GPUColorTargetInfo{
            .texture = color_targets[i].texture,
            .mip_level = 0,
            .layer_or_depth_plane = 0,
            .clear_color = color_targets[i].clear_color,
            .load_op = color_targets[i].load_op,
            .store_op = color_targets[i].store_op,
            .resolve_texture = nullptr,
            .resolve_mip_level = 0,
            .resolve_layer = 0,
            .cycle = false,
            .cycle_resolve_texture = false,
            .padding1 = 0,
            .padding2 = 0,
        };
    }

    auto* render_pass = SDL_BeginGPURenderPass(
        command_buffer,
        sdl_color_targets.data(),
        static_cast<uint32_t>(sdl_color_targets.size()),
        nullptr
    );

    return RenderPass{render_pass};
}

auto CommandBuffer::submit() -> void {
    Expects(command_buffer != nullptr);

    if (!SDL_SubmitGPUCommandBuffer(command_buffer)) {
        SDL_LogError(
            SDL_LOG_CATEGORY_ERROR,
            "Failed to submit GPU command buffer: %s",
            SDL_GetError()
        );
    }
    command_buffer = nullptr;
}

auto CommandBuffer::cancel() -> void {
    Expects(command_buffer != nullptr);

    SDL_CancelGPUCommandBuffer(command_buffer);
    command_buffer = nullptr;
}

}  // namespace Luminol::Graphics::SDL_GPU
