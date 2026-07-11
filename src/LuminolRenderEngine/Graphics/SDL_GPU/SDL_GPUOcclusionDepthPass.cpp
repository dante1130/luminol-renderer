#include "SDL_GPUOcclusionDepthPass.hpp"

#include <array>
#include <cstddef>
#include <filesystem>

#include <SDL3/SDL_video.h>

#include <LuminolMaths/Vector.hpp>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCommandBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUDevice.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUFactory.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUMesh.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPURenderPass.hpp>

namespace {

using namespace Luminol::Graphics::SDL_GPU;
using namespace Luminol::Maths;

constexpr auto vertex_stride_in_floats = 11U;
constexpr auto vertex_stride_in_bytes =
    sizeof(float) * vertex_stride_in_floats;

constexpr auto mesh_vertex_buffer_descriptions = std::array{
    VertexBufferDescription{
        .slot = 0,
        .pitch = vertex_stride_in_bytes,
    },
};

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
    VertexAttribute{
        .location = 2,
        .buffer_slot = 0,
        .format = VertexElementFormat::Float3,
        .offset = sizeof(float) * 5,
    },
    VertexAttribute{
        .location = 3,
        .buffer_slot = 0,
        .format = VertexElementFormat::Float3,
        .offset = sizeof(float) * 8,
    },
};

constexpr auto depth_format = TextureFormat::D24_Unorm;

// Mirrors cbuffer UBO in pbr_vert.hlsl.
struct VertexUBO {
    Matrix4x4f view_proj;
    std::array<uint32_t, 4> instance_base_offset;
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
    auto width = int{0};
    auto height = int{0};
    SDL_GetWindowSizeInPixels(window, &width, &height);

    return make_depth_texture(
        device, static_cast<uint32_t>(width), static_cast<uint32_t>(height)
    );
}

auto make_shader(
    GPUDevice& device,
    const std::filesystem::path& path,
    ShaderStage stage,
    uint32_t uniform_buffer_count,
    uint32_t storage_buffer_count
) -> Shader {
    return device.create_shader(ShaderInfo{
        .path = path,
        .stage = stage,
        .source_language = ShaderSourceLanguage::Hlsl,
        .sampler_count = 0U,
        .uniform_buffer_count = uniform_buffer_count,
        .storage_buffer_count = storage_buffer_count,
    });
}

auto make_pipeline(
    GPUDevice& device,
    const Shader& vertex_shader,
    const Shader& fragment_shader
) -> GraphicsPipeline {
    return device.create_graphics_pipeline(GraphicsPipelineInfo{
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_target_format = std::nullopt,
        .primitive_type = PrimitiveType::TriangleList,
        .vertex_buffer_descriptions = mesh_vertex_buffer_descriptions,
        .vertex_attributes = mesh_vertex_attributes,
        .enable_depth_test = true,
        .depth_stencil_format = depth_format,
        .cull_mode = CullMode::Back,
        .front_face = FrontFace::Clockwise,
    });
}

}  // namespace

namespace Luminol::Graphics::SDL_GPU {

SDL_GPUOcclusionDepthPass::SDL_GPUOcclusionDepthPass(
    GPUDevice& device, SDL_Window* window
)
    : vertex_shader{make_shader(
          device, "res/shaders/sdl_gpu/pbr_vert.hlsl", ShaderStage::Vertex,
          1U, 2U
      )},
      fragment_shader{make_shader(
          device, "res/shaders/sdl_gpu/shadow_depth_frag.hlsl",
          ShaderStage::Fragment, 0U, 0U
      )},
      pipeline{make_pipeline(device, vertex_shader, fragment_shader)},
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

    for (auto batch_index = std::size_t{0}; batch_index < instance_batches.size();
         ++batch_index) {
        const auto& batch = instance_batches[batch_index];
        const auto& submesh_infos = instance_cull_layout[batch_index];
        const auto meshes = graphics_factory.get_meshes(batch.renderable_id);

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

        for (auto mesh_index = std::size_t{0}; mesh_index < meshes.size();
             ++mesh_index) {
            const auto& info = submesh_infos[mesh_index];
            const auto vertex_ubo = VertexUBO{
                .view_proj = view_proj,
                .instance_base_offset = {info.instance_base_offset, 0, 0, 0},
            };
            command_buffer.push_vertex_uniform_data(
                0,
                gsl::span{
                    reinterpret_cast<const std::byte*>(&vertex_ubo),
                    sizeof(vertex_ubo)
                }
            );

            meshes[mesh_index].draw_indirect_geometry_only(
                render_pass, indirect_command_buffer,
                info.indirect_command_byte_offset
            );
        }
    }
}

auto SDL_GPUOcclusionDepthPass::get_depth_texture() const -> const Texture& {
    return depth_texture;
}

}  // namespace Luminol::Graphics::SDL_GPU
