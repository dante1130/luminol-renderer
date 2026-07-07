#include "SDL_GPURenderer.hpp"

#include <array>
#include <utility>

#include <gsl/gsl>
#include <SDL3/SDL_video.h>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCommandBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCopyPass.hpp>

namespace {

using namespace Luminol::Graphics::SDL_GPU;

constexpr auto depth_texture_format = TextureFormat::D24_Unorm;

auto make_depth_texture(GPUDevice& device, uint32_t width, uint32_t height)
    -> Texture {
    return device.create_texture(TextureInfo{
        .width = width,
        .height = height,
        .format = depth_texture_format,
        .usage = TextureUsage::DepthStencilTarget,
    });
}

auto make_depth_texture(GPUDevice& device, SDL_Window* window) -> Texture {
    auto width = int{0};
    auto height = int{0};
    SDL_GetWindowSizeInPixels(window, &width, &height);

    return make_depth_texture(
        device, static_cast<uint32_t>(width), static_cast<uint32_t>(height)
    );
}

}  // namespace

namespace Luminol::Graphics::SDL_GPU {

SDL_GPURenderer::SDL_GPURenderer(
    Window& window,
    std::shared_ptr<GraphicsFactory> graphics_factory,
    std::shared_ptr<GPUDevice> gpu_device
)
    : Renderer(std::move(graphics_factory)),
      sdl_window{static_cast<SDL_Window*>(window.get_window_handle())},
      gpu_device{std::move(gpu_device)},
      mesh_render_pass{*this->gpu_device, sdl_window},
      depth_texture{make_depth_texture(*this->gpu_device, sdl_window)} {}

auto SDL_GPURenderer::set_view_matrix(const Maths::Matrix4x4f& view_matrix)
    -> void {
    this->view_matrix = view_matrix;
}

auto SDL_GPURenderer::set_projection_matrix(
    const Maths::Matrix4x4f& projection_matrix
) -> void {
    this->projection_matrix = projection_matrix;
}

auto SDL_GPURenderer::set_exposure(float /*exposure*/) -> void {}

auto SDL_GPURenderer::set_luminance_heatmap_enabled(bool enabled) -> void {
    luminance_heatmap_enabled = enabled;
}

auto SDL_GPURenderer::get_luminance_heatmap_enabled() const -> bool {
    return luminance_heatmap_enabled;
}

auto SDL_GPURenderer::clear_color(const Maths::Vector4f& color) const -> void {
    clear_color_value = color;
}

// SDL_GPU handles the clear via LOADOP_CLEAR inside begin_render_pass, so
// there is nothing to do here; the color set in clear_color is applied at that
// point instead.
auto SDL_GPURenderer::clear(BufferBit /*buffer_bit*/) const -> void {}

auto SDL_GPURenderer::queue_draw(
    RenderableId renderable_id, const Maths::Matrix4x4f& model_matrix
) -> void {
    queued_draws[renderable_id].push_back(model_matrix);
}

auto SDL_GPURenderer::queue_draw_instanced(
    RenderableId renderable_id, gsl::span<const Maths::Matrix4x4f> model_matrices
) -> void {
    auto& batch = queued_draws[renderable_id];
    batch.insert(batch.end(), model_matrices.begin(), model_matrices.end());
}

auto SDL_GPURenderer::queue_draw_with_color(
    RenderableId /*renderable_id*/,
    const Maths::Matrix4x4f& /*model_matrix*/,
    const Maths::Vector3f& /*color*/
) -> void {}

auto SDL_GPURenderer::queue_draw_line(
    const Maths::Vector3f& /*start*/,
    const Maths::Vector3f& /*end*/,
    const Maths::Vector3f& /*color*/
) -> void {}

auto SDL_GPURenderer::draw() -> void {
    auto command_buffer = gpu_device->create_command_buffer();

    const auto swapchain = command_buffer.acquire_swapchain_texture(sdl_window);
    if (!swapchain.has_value()) {
        command_buffer.submit();
        queued_draws.clear();
        return;
    }

    const auto color_targets = std::array{ColorTargetInfo{
        .texture = &swapchain->texture,
        .clear_color = clear_color_value,
        .load_op = LoadOp::Clear,
        .store_op = StoreOp::Store,
    }};

    if (depth_texture.get_width() != swapchain->width ||
        depth_texture.get_height() != swapchain->height) {
        depth_texture = make_depth_texture(
            *gpu_device, swapchain->width, swapchain->height
        );
    }

    const auto depth_texture_view = TextureView{depth_texture.native_handle()};
    const auto depth_stencil_target = DepthStencilTargetInfo{
        .texture = &depth_texture_view,
        .clear_depth = 1.0F,
        .load_op = LoadOp::Clear,
        .store_op = StoreOp::DontCare,
    };

    auto instance_batches = std::vector<InstanceBatch>{};
    {
        auto copy_pass = command_buffer.begin_copy_pass();
        instance_batches =
            mesh_render_pass.upload_instances(*gpu_device, copy_pass, queued_draws);
    }

    {
        auto render_pass = command_buffer.begin_render_pass(
            color_targets, &depth_stencil_target
        );

        mesh_render_pass.draw(
            this->get_renderable_manager(),
            command_buffer,
            render_pass,
            instance_batches,
            view_matrix * projection_matrix
        );
    }

    command_buffer.submit();
    queued_draws.clear();
}

}  // namespace Luminol::Graphics::SDL_GPU
