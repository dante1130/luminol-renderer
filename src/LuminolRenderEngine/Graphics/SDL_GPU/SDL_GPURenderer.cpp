#include "SDL_GPURenderer.hpp"

#include <array>
#include <utility>

#include <gsl/gsl>
#include <SDL3/SDL_video.h>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCommandBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCopyPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUFactory.hpp>
#include <LuminolRenderEngine/Utilities/Timer.hpp>

namespace {

using namespace Luminol::Graphics::SDL_GPU;
using namespace Luminol::Maths;

constexpr auto depth_texture_format = TextureFormat::D24_Unorm;
constexpr auto hdr_color_texture_format = TextureFormat::R16G16B16A16_Float;

auto get_view_position(const Matrix4x4f& view_matrix) -> Vector4f {
    const auto inverse_view_matrix = view_matrix.inverse();

    return Vector4f{
        inverse_view_matrix[3][0],
        inverse_view_matrix[3][1],
        inverse_view_matrix[3][2],
        0.0F,
    };
}

auto make_depth_texture(GPUDevice& device, uint32_t width, uint32_t height)
    -> Texture {
    return device.create_texture(TextureInfo{
        .width = width,
        .height = height,
        .format = depth_texture_format,
        .usage = TextureUsage::DepthStencilTarget | TextureUsage::Sampler,
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

auto make_hdr_color_texture(GPUDevice& device, uint32_t width, uint32_t height)
    -> Texture {
    return device.create_texture(TextureInfo{
        .width = width,
        .height = height,
        .format = hdr_color_texture_format,
        .usage = TextureUsage::ColorTarget | TextureUsage::Sampler,
    });
}

auto make_hdr_color_texture(GPUDevice& device, SDL_Window* window) -> Texture {
    auto width = int{0};
    auto height = int{0};
    SDL_GetWindowSizeInPixels(window, &width, &height);

    return make_hdr_color_texture(
        device, static_cast<uint32_t>(width), static_cast<uint32_t>(height)
    );
}

}  // namespace

namespace Luminol::Graphics::SDL_GPU {

SDL_GPURenderer::SDL_GPURenderer(
    Window& window,
    std::shared_ptr<SDL_GPUFactory> graphics_factory,
    std::shared_ptr<GPUDevice> gpu_device
)
    : Renderer(graphics_factory),
      sdl_window{static_cast<SDL_Window*>(window.get_window_handle())},
      sdl_gpu_factory{std::move(graphics_factory)},
      gpu_device{std::move(gpu_device)},
      mesh_render_pass{*this->gpu_device},
      ao_pass{*this->gpu_device, sdl_window},
      tonemap_pass{*this->gpu_device, sdl_window},
      depth_texture{make_depth_texture(*this->gpu_device, sdl_window)},
      hdr_color_texture{make_hdr_color_texture(*this->gpu_device, sdl_window)} {}

auto SDL_GPURenderer::set_view_matrix(const Maths::Matrix4x4f& view_matrix)
    -> void {
    this->view_matrix = view_matrix;
}

auto SDL_GPURenderer::set_projection_matrix(
    const Maths::Matrix4x4f& projection_matrix
) -> void {
    this->projection_matrix = projection_matrix;
}

auto SDL_GPURenderer::set_exposure(float exposure) -> void {
    this->exposure = exposure;
}

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
    const auto frame_timer = Utilities::Timer{};

    auto command_buffer = gpu_device->create_command_buffer();

    const auto acquire_timer = Utilities::Timer{};
    const auto swapchain = command_buffer.acquire_swapchain_texture(sdl_window);
    performance_logger.record(
        "acquire_swapchain", Units::Seconds{acquire_timer.elapsed_seconds()}
    );

    if (!swapchain.has_value()) {
        command_buffer.submit();
        queued_draws.clear();
        return;
    }

    if (depth_texture.get_width() != swapchain->width ||
        depth_texture.get_height() != swapchain->height) {
        depth_texture = make_depth_texture(
            *gpu_device, swapchain->width, swapchain->height
        );
        hdr_color_texture = make_hdr_color_texture(
            *gpu_device, swapchain->width, swapchain->height
        );
        ao_pass.resize(*gpu_device, swapchain->width, swapchain->height);
    }

    const auto depth_texture_view = TextureView{depth_texture.native_handle()};
    const auto hdr_color_texture_view =
        TextureView{hdr_color_texture.native_handle()};

    const auto color_targets = std::array{ColorTargetInfo{
        .texture = &hdr_color_texture_view,
        .clear_color = clear_color_value,
        .load_op = LoadOp::Clear,
        .store_op = StoreOp::Store,
    }};

    auto instance_batches = std::vector<InstanceBatch>{};
    {
        const auto pass_timer = Utilities::Timer{};

        auto copy_pass = command_buffer.begin_copy_pass();
        instance_batches =
            mesh_render_pass.upload_instances(*gpu_device, copy_pass, queued_draws);

        performance_logger.record(
            "instance_upload", Units::Seconds{pass_timer.elapsed_seconds()}
        );
    }

    ao_pass.draw(
        *this->sdl_gpu_factory,
        command_buffer,
        mesh_render_pass.get_instance_buffer_cache(),
        instance_batches,
        view_matrix,
        projection_matrix,
        depth_texture,
        performance_logger
    );

    const auto depth_stencil_target = DepthStencilTargetInfo{
        .texture = &depth_texture_view,
        .clear_depth = 1.0F,
        .load_op = LoadOp::Load,
        .store_op = StoreOp::DontCare,
    };

    {
        const auto pass_timer = Utilities::Timer{};

        auto render_pass = command_buffer.begin_render_pass(
            color_targets, &depth_stencil_target
        );

        const auto& directional_light =
            get_light_manager().get_light_data().directional_light;

        const auto light_data = DirectionalLightData{
            .direction = directional_light.direction,
            .color = directional_light.color,
            .view_position = get_view_position(view_matrix),
            .screen_size = Maths::Vector4f{
                static_cast<float>(swapchain->width),
                static_cast<float>(swapchain->height),
                0.0F,
                0.0F,
            },
        };

        mesh_render_pass.draw(
            *this->sdl_gpu_factory,
            command_buffer,
            render_pass,
            instance_batches,
            view_matrix * projection_matrix,
            light_data,
            ao_pass.get_ao_texture(),
            ao_pass.get_sampler()
        );

        performance_logger.record(
            "main_pass", Units::Seconds{pass_timer.elapsed_seconds()}
        );
    }

    {
        const auto pass_timer = Utilities::Timer{};

        const auto tonemap_color_targets = std::array{ColorTargetInfo{
            .texture = &swapchain->texture,
            .load_op = LoadOp::DontCare,
            .store_op = StoreOp::Store,
        }};

        auto tonemap_render_pass =
            command_buffer.begin_render_pass(tonemap_color_targets);

        tonemap_pass.draw(
            command_buffer, tonemap_render_pass, hdr_color_texture, exposure
        );

        performance_logger.record(
            "tonemap", Units::Seconds{pass_timer.elapsed_seconds()}
        );
    }

    command_buffer.submit();
    queued_draws.clear();

    performance_logger.record(
        "frame", Units::Seconds{frame_timer.elapsed_seconds()}
    );
    performance_logger.end_frame();
}

}  // namespace Luminol::Graphics::SDL_GPU
