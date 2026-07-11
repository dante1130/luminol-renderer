#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

#include <gsl/gsl>
#include <LuminolMaths/Vector.hpp>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUComputePass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCopyPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPURenderPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTexture.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTypes.hpp>

struct SDL_GPUCommandBuffer;
struct SDL_Window;

namespace Luminol::Graphics::SDL_GPU {

class Buffer;

struct SwapchainTexture {
    TextureView texture;
    uint32_t width;
    uint32_t height;
};

struct StorageBufferReadWriteBinding {
    // Required; asserted non-null in begin_compute_pass. Pointer (not
    // reference) so callers can use designated-initializer syntax.
    const Buffer* buffer = nullptr;
    bool cycle = false;
};

struct StorageTextureReadWriteBinding {
    // Required; asserted non-null in begin_compute_pass. Pointer (not
    // reference) so callers can use designated-initializer syntax.
    const Texture* texture = nullptr;
    uint32_t mip_level = 0;
    uint32_t layer = 0;
    bool cycle = false;
};

struct ColorTargetInfo {
    // Required; asserted non-null in begin_render_pass. Pointer (not
    // reference) so callers can use designated-initializer syntax.
    const TextureView* texture = nullptr;
    Maths::Vector4f clear_color = {0.0F, 0.0F, 0.0F, 1.0F};
    LoadOp load_op = LoadOp::Clear;
    StoreOp store_op = StoreOp::Store;
    bool cycle = false;
    // Required when store_op is Resolve/ResolveAndStore; asserted non-null in
    // begin_render_pass for those store ops. Pointer (not reference) so
    // callers can use designated-initializer syntax, matching `texture`.
    const TextureView* resolve_texture = nullptr;
    bool cycle_resolve_texture = false;
    // Selects which mip level / cubemap face (or array layer) of `texture`
    // to render into. Needed for precompute passes that render each face and
    // mip of a cubemap individually (e.g. irradiance convolution,
    // prefiltered specular).
    uint32_t mip_level = 0;
    uint32_t layer = 0;
};

struct DepthStencilTargetInfo {
    const TextureView* texture = nullptr;
    float clear_depth = 1.0F;
    LoadOp load_op = LoadOp::Clear;
    StoreOp store_op = StoreOp::Store;
    bool cycle = false;
    // Selects which mip level / array layer (or cubemap face, for a
    // TextureCubeArray this is face_index + 6 * array_index) of `texture` to
    // render into. Mirrors ColorTargetInfo::mip_level/layer.
    uint32_t mip_level = 0;
    uint32_t layer = 0;
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

    // storage_texture_bindings textures must have been created with a
    // TextureUsage::ComputeStorage* flag; storage_buffer_bindings buffers
    // must have been created with BufferUsage::ComputeStorageReadWrite.
    // Asserted non-null in begin_compute_pass.
    [[nodiscard]] auto begin_compute_pass(
        gsl::span<const StorageTextureReadWriteBinding> storage_texture_bindings,
        gsl::span<const StorageBufferReadWriteBinding> storage_buffer_bindings
    ) -> ComputePass;

    // Must not be called while any render/copy pass on this command buffer
    // is open. texture must have been created with more than 1 mip level.
    auto generate_mipmaps(const Texture& texture) -> void;

    auto push_vertex_uniform_data(
        uint32_t slot, gsl::span<const std::byte> data
    ) -> void;

    auto push_fragment_uniform_data(
        uint32_t slot, gsl::span<const std::byte> data
    ) -> void;

    auto push_compute_uniform_data(
        uint32_t slot, gsl::span<const std::byte> data
    ) -> void;

    auto submit() -> void;
    auto cancel() -> void;

private:
    SDL_GPUCommandBuffer* command_buffer;
};

}  // namespace Luminol::Graphics::SDL_GPU
