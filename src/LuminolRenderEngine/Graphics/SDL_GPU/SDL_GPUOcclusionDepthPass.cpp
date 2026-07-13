#include "SDL_GPUOcclusionDepthPass.hpp"

#include <array>
#include <cstddef>

#include <SDL3/SDL_video.h>

#include <LuminolMaths/Vector.hpp>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCommandBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUDevice.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUFactory.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPURenderPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUResourceBuilders.hpp>

namespace {

using namespace Luminol::Graphics::SDL_GPU;
using namespace Luminol::Maths;

constexpr auto depth_format = TextureFormat::D24_Unorm;

// Mirrors cbuffer UBO in pbr_vert.hlsl.
struct VertexUBO {
    Matrix4x4f view_proj;
};

auto make_depth_texture(GPUDevice& device, uint32_t width, uint32_t height)
    -> Texture {
    return device.create_texture(TextureInfo{
        .width = width,
        .height = height,
        .format = depth_format,
        .usage = TextureUsage::DepthStencilTarget | TextureUsage::Sampler,
    });
}

auto make_depth_texture(GPUDevice& device, SDL_Window* window) -> Texture {
    const auto [width, height] = get_window_size_in_pixels(window);
    return make_depth_texture(device, width, height);
}

}  // namespace

namespace Luminol::Graphics::SDL_GPU {

SDL_GPUOcclusionDepthPass::SDL_GPUOcclusionDepthPass(
    GPUDevice& device, SDL_Window* window
)
    : vertex_shader{make_hlsl_shader(
          device, "res/shaders/sdl_gpu/pbr_vert.hlsl", ShaderStage::Vertex,
          0U, 1U, 2U
      )},
      fragment_shader{make_hlsl_shader(
          device, "res/shaders/sdl_gpu/shadow_depth_frag.hlsl",
          ShaderStage::Fragment
      )},
      pipeline{
          make_depth_only_mesh_pipeline(device, vertex_shader, fragment_shader, depth_format)
      },
      depth_texture{make_depth_texture(device, window)} {}

auto SDL_GPUOcclusionDepthPass::resize(
    GPUDevice& device, uint32_t width, uint32_t height
) -> void {
    depth_texture = make_depth_texture(device, width, height);
}

auto SDL_GPUOcclusionDepthPass::draw(
    const SDL_GPUFactory& graphics_factory,
    CommandBuffer& command_buffer,
    const SDL_GPUInstanceBufferCache& instance_buffer_cache,
    gsl::span<const InstanceBatch> instance_batches,
    const Maths::Matrix4x4f& view_matrix,
    const Maths::Matrix4x4f& projection_matrix,
    const Buffer& indirect_command_buffer,
    const Buffer& visible_instance_indices_buffer,
    const InstanceCullLayout& instance_cull_layout
) -> void {
    const auto depth_texture_view = TextureView{depth_texture.native_handle()};

    const auto depth_stencil_target = DepthStencilTargetInfo{
        .texture = &depth_texture_view,
        .clear_depth = 1.0F,
        .load_op = LoadOp::Clear,
        .store_op = StoreOp::Store,
        .cycle = true,
    };

    auto render_pass = command_buffer.begin_render_pass({}, &depth_stencil_target);
    render_pass.bind_graphics_pipeline(pipeline);

    const auto view_proj = view_matrix * projection_matrix;
    const auto vertex_ubo = VertexUBO{.view_proj = view_proj};
    command_buffer.push_vertex_uniform_data(
        0,
        gsl::span{
            reinterpret_cast<const std::byte*>(&vertex_ubo), sizeof(vertex_ubo)
        }
    );

    for (auto batch_index = std::size_t{0}; batch_index < instance_batches.size();
         ++batch_index) {
        const auto& batch = instance_batches[batch_index];
        const auto& submesh_infos = instance_cull_layout[batch_index];

        if (submesh_infos.empty()) {
            continue;
        }

        const auto& instance_buffer =
            instance_buffer_cache.get(batch.renderable_id);
        const auto storage_buffer_bindings =
            std::array{&instance_buffer, &visible_instance_indices_buffer};
        render_pass.bind_vertex_storage_buffers(0, storage_buffer_bindings);

        const auto vertex_bindings = std::array{VertexBufferBinding{
            .buffer = &graphics_factory.get_vertex_buffer(batch.renderable_id),
            .offset = 0,
        }};
        render_pass.bind_vertex_buffers(0, vertex_bindings);
        render_pass.bind_index_buffer(
            graphics_factory.get_index_buffer(batch.renderable_id),
            IndexElementSize::Bits32, 0
        );

        // Every submesh in this batch's IndirectDrawCommand slice is
        // contiguous (SDL_GPUInstanceCullPass::cull builds them that way),
        // and each carries its own visible_instance_indices base offset via
        // first_instance, so one multi-draw call covers the whole batch
        // instead of one draw per submesh.
        render_pass.draw_indexed_primitives_indirect(
            indirect_command_buffer,
            submesh_infos.front().indirect_command_byte_offset,
            static_cast<uint32_t>(submesh_infos.size())
        );
    }
}

auto SDL_GPUOcclusionDepthPass::get_depth_texture() const -> const Texture& {
    return depth_texture;
}

}  // namespace Luminol::Graphics::SDL_GPU
