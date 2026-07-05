#pragma once

#include <cstdint>
#include <optional>

#include <gsl/gsl>
#include <LuminolMaths/Vector.hpp>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPURenderPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTexture.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTypes.hpp>

struct SDL_GPUCommandBuffer;
struct SDL_Window;

namespace Luminol::Graphics::SDL_GPU {

struct SwapchainTexture {
    Texture texture;
    uint32_t width;
    uint32_t height;
};

struct ColorTargetInfo {
    const Texture* texture = nullptr;
    Maths::Vector4f clear_color = {0.0F, 0.0F, 0.0F, 1.0F};
    LoadOp load_op = LoadOp::Clear;
    StoreOp store_op = StoreOp::Store;
};

class CommandBuffer {
public:
    CommandBuffer(SDL_GPUCommandBuffer* command_buffer);
    ~CommandBuffer();

    CommandBuffer(const CommandBuffer&) = delete;
    auto operator=(const CommandBuffer&) -> CommandBuffer& = delete;

    CommandBuffer(CommandBuffer&& other) noexcept;
    auto operator=(CommandBuffer&& other) noexcept -> CommandBuffer&;

    // SDL_Window* is a deliberate exception: Luminol does not wrap the window
    // yet, so callers pass the SDL handle through.
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
