#include "SDL_GPURenderer.hpp"

#include <array>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCommandBuffer.hpp>

namespace {

using namespace Luminol::Graphics::SDL_GPU;

auto make_triangle_shader(
    GPUDevice& device, const std::filesystem::path& path, ShaderStage stage
) -> Shader {
    return device.create_shader(ShaderInfo{
        .path = path,
        .stage = stage,
        .source_language = ShaderSourceLanguage::Hlsl,
    });
}

auto make_triangle_pipeline(
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
        .vertex_buffer_descriptions = {},
        .vertex_attributes = {},
    });
}

}  // namespace

namespace Luminol::Graphics::SDL_GPU {

SDL_GPURenderer::SDL_GPURenderer(Window& window, GraphicsApi graphics_api)
    : Renderer(graphics_api),
      sdl_window{static_cast<SDL_Window*>(window.get_window_handle())},
      get_window_width([&window]() { return window.get_width(); }),
      get_window_height([&window]() { return window.get_height(); }),
      gpu_device{sdl_window},
      triangle_vertex_shader{make_triangle_shader(
          gpu_device,
          "res/shaders/sdl_gpu/triangle_vert.hlsl",
          ShaderStage::Vertex
      )},
      triangle_fragment_shader{make_triangle_shader(
          gpu_device,
          "res/shaders/sdl_gpu/triangle_frag.hlsl",
          ShaderStage::Fragment
      )},
      triangle_pipeline{make_triangle_pipeline(
          gpu_device,
          sdl_window,
          triangle_vertex_shader,
          triangle_fragment_shader
      )} {}

auto SDL_GPURenderer::set_view_matrix(const Maths::Matrix4x4f& /*view_matrix*/)
    -> void {}

auto SDL_GPURenderer::set_projection_matrix(
    const Maths::Matrix4x4f& /*projection_matrix*/
) -> void {}

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

auto SDL_GPURenderer::clear(BufferBit /*buffer_bit*/) const -> void {}

auto SDL_GPURenderer::queue_draw(
    RenderableId /*renderable_id*/, const Maths::Matrix4x4f& /*model_matrix*/
) -> void {}

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
    auto command_buffer = gpu_device.create_command_buffer();

    const auto swapchain = command_buffer.acquire_swapchain_texture(sdl_window);
    if (!swapchain.has_value()) {
        command_buffer.submit();
        return;
    }

    const auto color_targets = std::array{ColorTargetInfo{
        .texture = &swapchain->texture,
        .clear_color = clear_color_value,
        .load_op = LoadOp::Clear,
        .store_op = StoreOp::Store,
    }};

    {
        auto render_pass = command_buffer.begin_render_pass(color_targets);
        render_pass.bind_graphics_pipeline(triangle_pipeline);
        render_pass.draw_primitives(3);
    }

    command_buffer.submit();
}

}  // namespace Luminol::Graphics::SDL_GPU
