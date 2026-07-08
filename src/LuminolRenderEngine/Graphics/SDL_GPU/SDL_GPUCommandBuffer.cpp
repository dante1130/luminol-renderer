#include "SDL_GPUCommandBuffer.hpp"

#include <vector>

#include <gsl/gsl>

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_log.h>

namespace {

using namespace Luminol::Graphics::SDL_GPU;

constexpr auto to_sdl_load_op(LoadOp op) -> SDL_GPULoadOp {
    switch (op) {
        case LoadOp::Load:
            return SDL_GPU_LOADOP_LOAD;
        case LoadOp::Clear:
            return SDL_GPU_LOADOP_CLEAR;
        case LoadOp::DontCare:
            return SDL_GPU_LOADOP_DONT_CARE;
    }
    throw std::runtime_error{"Invalid load op"};
}

constexpr auto to_sdl_store_op(StoreOp op) -> SDL_GPUStoreOp {
    switch (op) {
        case StoreOp::Store:
            return SDL_GPU_STOREOP_STORE;
        case StoreOp::DontCare:
            return SDL_GPU_STOREOP_DONT_CARE;
        case StoreOp::Resolve:
            return SDL_GPU_STOREOP_RESOLVE;
        case StoreOp::ResolveAndStore:
            return SDL_GPU_STOREOP_RESOLVE_AND_STORE;
    }
    throw std::runtime_error{"Invalid store op"};
}

}  // namespace

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
        .texture = TextureView{texture}, .width = width, .height = height
    };
}

auto CommandBuffer::begin_render_pass(
    gsl::span<const ColorTargetInfo> color_targets,
    const DepthStencilTargetInfo* depth_stencil_target
) -> RenderPass {
    Expects(command_buffer != nullptr);

    auto sdl_color_targets =
        std::vector<SDL_GPUColorTargetInfo>(color_targets.size());
    for (auto i = size_t{0}; i < color_targets.size(); ++i) {
        const auto& target = color_targets[i];
        Expects(target.texture != nullptr);
        sdl_color_targets[i] = SDL_GPUColorTargetInfo{
            .texture = target.texture->native_handle(),
            .mip_level = 0,
            .layer_or_depth_plane = 0,
            .clear_color =
                SDL_FColor{
                    .r = target.clear_color.x(),
                    .g = target.clear_color.y(),
                    .b = target.clear_color.z(),
                    .a = target.clear_color.w(),
                },
            .load_op = to_sdl_load_op(target.load_op),
            .store_op = to_sdl_store_op(target.store_op),
            .resolve_texture = nullptr,
            .resolve_mip_level = 0,
            .resolve_layer = 0,
            .cycle = target.cycle,
            .cycle_resolve_texture = false,
            .padding1 = 0,
            .padding2 = 0,
        };
    }

    auto sdl_depth_stencil_target = SDL_GPUDepthStencilTargetInfo{};
    if (depth_stencil_target != nullptr) {
        Expects(depth_stencil_target->texture != nullptr);
        sdl_depth_stencil_target = SDL_GPUDepthStencilTargetInfo{
            .texture = depth_stencil_target->texture->native_handle(),
            .clear_depth = depth_stencil_target->clear_depth,
            .load_op = to_sdl_load_op(depth_stencil_target->load_op),
            .store_op = to_sdl_store_op(depth_stencil_target->store_op),
            .stencil_load_op = SDL_GPU_LOADOP_DONT_CARE,
            .stencil_store_op = SDL_GPU_STOREOP_DONT_CARE,
            .cycle = depth_stencil_target->cycle,
            .clear_stencil = 0,
        };
    }

    auto* render_pass = SDL_BeginGPURenderPass(
        command_buffer,
        sdl_color_targets.data(),
        static_cast<uint32_t>(sdl_color_targets.size()),
        depth_stencil_target != nullptr ? &sdl_depth_stencil_target : nullptr
    );

    return RenderPass{render_pass};
}

auto CommandBuffer::push_vertex_uniform_data(
    uint32_t slot, gsl::span<const std::byte> data
) -> void {
    Expects(command_buffer != nullptr);
    SDL_PushGPUVertexUniformData(
        command_buffer,
        slot,
        data.data(),
        static_cast<uint32_t>(data.size())
    );
}

auto CommandBuffer::push_fragment_uniform_data(
    uint32_t slot, gsl::span<const std::byte> data
) -> void {
    Expects(command_buffer != nullptr);
    SDL_PushGPUFragmentUniformData(
        command_buffer,
        slot,
        data.data(),
        static_cast<uint32_t>(data.size())
    );
}

auto CommandBuffer::begin_copy_pass() -> CopyPass {
    Expects(command_buffer != nullptr);
    return CopyPass{SDL_BeginGPUCopyPass(command_buffer)};
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
