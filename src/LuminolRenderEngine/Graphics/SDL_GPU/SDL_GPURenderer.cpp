#include "SDL_GPURenderer.hpp"

#include <array>
#include <cstddef>
#include <utility>

#include <gsl/gsl>
#include <SDL3/SDL_video.h>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCommandBuffer.hpp>

namespace {

using namespace Luminol::Graphics::SDL_GPU;

constexpr auto vertex_stride_in_floats = 11U;
constexpr auto vertex_stride_in_bytes =
    sizeof(float) * vertex_stride_in_floats;

constexpr auto mesh_vertex_buffer_descriptions =
    std::array{VertexBufferDescription{
        .slot = 0,
        .pitch = vertex_stride_in_bytes,
    }};

constexpr auto mesh_vertex_attributes = std::array{
    VertexAttribute{
        .location = 0,
        .buffer_slot = 0,
        .format = VertexElementFormat::Float3,
        .offset = 0,
    },
    VertexAttribute{
        .location = 1,
        .buffer_slot = 0,
        .format = VertexElementFormat::Float2,
        .offset = sizeof(float) * 3,
    },
};

constexpr auto depth_texture_format = TextureFormat::D24_Unorm;

auto make_mesh_shader(
    GPUDevice& device, const std::filesystem::path& path, ShaderStage stage
) -> Shader {
    return device.create_shader(ShaderInfo{
        .path = path,
        .stage = stage,
        .source_language = ShaderSourceLanguage::Hlsl,
        .sampler_count = (stage == ShaderStage::Fragment) ? 1U : 0U,
        .uniform_buffer_count = (stage == ShaderStage::Vertex) ? 1U : 0U,
    });
}

auto make_mesh_pipeline(
    GPUDevice& device,
    SDL_Window* window,
    const Shader& vertex_shader,
    const Shader& fragment_shader
) -> GraphicsPipeline {
    return device.create_graphics_pipeline(GraphicsPipelineInfo{
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_target_format = device.get_swapchain_texture_format(window),
        .primitive_type = PrimitiveType::TriangleList,
        .vertex_buffer_descriptions = mesh_vertex_buffer_descriptions,
        .vertex_attributes = mesh_vertex_attributes,
        .enable_depth_test = true,
        .depth_stencil_format = depth_texture_format,
        .cull_mode = CullMode::Back,
        .front_face = FrontFace::Clockwise,
    });
}

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
      mesh_vertex_shader{make_mesh_shader(
          *this->gpu_device,
          "res/shaders/sdl_gpu/mesh_vert.hlsl",
          ShaderStage::Vertex
      )},
      mesh_fragment_shader{make_mesh_shader(
          *this->gpu_device,
          "res/shaders/sdl_gpu/mesh_frag.hlsl",
          ShaderStage::Fragment
      )},
      mesh_pipeline{make_mesh_pipeline(
          *this->gpu_device,
          sdl_window,
          mesh_vertex_shader,
          mesh_fragment_shader
      )},
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
    queued_draws.push_back(QueuedDraw{
        .renderable_id = renderable_id, .model_matrix = model_matrix
    });
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

    {
        auto render_pass = command_buffer.begin_render_pass(
            color_targets, &depth_stencil_target
        );
        render_pass.bind_graphics_pipeline(mesh_pipeline);

        for (const auto& queued : queued_draws) {
            const auto& renderable =
                this->get_renderable_manager().get_renderable(
                    queued.renderable_id
                );

            const auto mvp = queued.model_matrix * view_matrix *
                              projection_matrix;
            command_buffer.push_vertex_uniform_data(
                0,
                gsl::span{
                    reinterpret_cast<const std::byte*>(&mvp), sizeof(mvp)
                }
            );

            renderable.draw(render_pass);
        }
    }

    command_buffer.submit();
    queued_draws.clear();
}

}  // namespace Luminol::Graphics::SDL_GPU
