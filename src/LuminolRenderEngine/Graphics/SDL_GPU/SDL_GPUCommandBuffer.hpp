#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

#include <gsl/gsl>
#include <LuminolMaths/Vector.hpp>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCopyPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPURenderPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTexture.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTypes.hpp>

struct SDL_GPUCommandBuffer;
struct SDL_Window;

namespace Luminol::Graphics::SDL_GPU {

struct SwapchainTexture {
    TextureView texture;
    uint32_t width;
    uint32_t height;
};

struct ColorTargetInfo {
    // Required; asserted non-null in begin_render_pass. Pointer (not
    // reference) so callers can use designated-initializer syntax.
    const TextureView* texture = nullptr;
    Maths::Vector4f clear_color = {0.0F, 0.0F, 0.0F, 1.0F};
    LoadOp load_op = LoadOp::Clear;
    StoreOp store_op = StoreOp::Store;
    bool cycle = false;
};

struct DepthStencilTargetInfo {
    const TextureView* texture = nullptr;
    float clear_depth = 1.0F;
    LoadOp load_op = LoadOp::Clear;
    StoreOp store_op = StoreOp::Store;
    bool cycle = false;
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
        gsl::span<const ColorTargetInfo> color_targets,
        const DepthStencilTargetInfo* depth_stencil_target = nullptr
    ) -> RenderPass;

    [[nodiscard]] auto begin_copy_pass() -> CopyPass;

    auto push_vertex_uniform_data(
        uint32_t slot, gsl::span<const std::byte> data
    ) -> void;

    auto push_fragment_uniform_data(
        uint32_t slot, gsl::span<const std::byte> data
    ) -> void;

    auto submit() -> void;
    auto cancel() -> void;

private:
    SDL_GPUCommandBuffer* command_buffer;
};

}  // namespace Luminol::Graphics::SDL_GPU
