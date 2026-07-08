#include "SDL_GPUMeshRenderPass.hpp"

#include <array>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCommandBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUDevice.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUFactory.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPURenderPass.hpp>

namespace {

using namespace Luminol::Graphics::SDL_GPU;

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

constexpr auto depth_texture_format = TextureFormat::D24_Unorm;
constexpr auto hdr_color_texture_format = TextureFormat::R16G16B16A16_Float;

constexpr auto fragment_sampler_count = 6U;
constexpr auto ssao_sampler_slot = 5U;

auto make_mesh_shader(
    GPUDevice& device, const std::filesystem::path& path, ShaderStage stage
) -> Shader {
    return device.create_shader(ShaderInfo{
        .path = path,
        .stage = stage,
        .source_language = ShaderSourceLanguage::Hlsl,
        .sampler_count =
            (stage == ShaderStage::Fragment) ? fragment_sampler_count : 0U,
        .uniform_buffer_count = 1U,
        .storage_buffer_count = (stage == ShaderStage::Vertex) ? 1U : 0U,
    });
}

auto make_mesh_pipeline(
    GPUDevice& device,
    const Shader& vertex_shader,
    const Shader& fragment_shader
) -> GraphicsPipeline {
    return device.create_graphics_pipeline(GraphicsPipelineInfo{
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_target_format = hdr_color_texture_format,
        .primitive_type = PrimitiveType::TriangleList,
        .vertex_buffer_descriptions = mesh_vertex_buffer_descriptions,
        .vertex_attributes = mesh_vertex_attributes,
        .enable_depth_test = true,
        .depth_stencil_format = depth_texture_format,
        .cull_mode = CullMode::Back,
        .front_face = FrontFace::Clockwise,
    });
}

}  // namespace

namespace Luminol::Graphics::SDL_GPU {

SDL_GPUMeshRenderPass::SDL_GPUMeshRenderPass(GPUDevice& device)
    : mesh_vertex_shader{make_mesh_shader(
          device, "res/shaders/sdl_gpu/pbr_vert.hlsl", ShaderStage::Vertex
      )},
      mesh_fragment_shader{make_mesh_shader(
          device, "res/shaders/sdl_gpu/pbr_frag.hlsl", ShaderStage::Fragment
      )},
      mesh_pipeline{make_mesh_pipeline(
          device, mesh_vertex_shader, mesh_fragment_shader
      )} {}

auto SDL_GPUMeshRenderPass::get_instance_buffer_cache() const
    -> const SDL_GPUInstanceBufferCache& {
    return instance_buffer_cache;
}

auto SDL_GPUMeshRenderPass::upload_instances(
    GPUDevice& device,
    CopyPass& copy_pass,
    const std::unordered_map<RenderableId, std::vector<Maths::Matrix4x4f>>&
        queued_draws
) -> std::vector<InstanceBatch> {
    auto instance_batches = std::vector<InstanceBatch>{};
    instance_batches.reserve(queued_draws.size());

    for (const auto& [renderable_id, model_matrices] : queued_draws) {
        instance_buffer_cache.upload(
            device, copy_pass, renderable_id, model_matrices
        );

        instance_batches.push_back(InstanceBatch{
            .renderable_id = renderable_id,
            .instance_count = static_cast<uint32_t>(model_matrices.size()),
        });
    }

    return instance_batches;
}

auto SDL_GPUMeshRenderPass::draw(
    const SDL_GPUFactory& graphics_factory,
    CommandBuffer& command_buffer,
    RenderPass& render_pass,
    gsl::span<const InstanceBatch> instance_batches,
    const Maths::Matrix4x4f& view_proj,
    const DirectionalLightData& light_data,
    const Texture& ssao_texture,
    const Sampler& ssao_sampler
) -> void {
    render_pass.bind_graphics_pipeline(mesh_pipeline);

    command_buffer.push_vertex_uniform_data(
        0,
        gsl::span{
            reinterpret_cast<const std::byte*>(&view_proj), sizeof(view_proj)
        }
    );

    command_buffer.push_fragment_uniform_data(
        0,
        gsl::span{
            reinterpret_cast<const std::byte*>(&light_data), sizeof(light_data)
        }
    );

    const auto ssao_sampler_bindings = std::array{TextureSamplerBinding{
        .texture = &ssao_texture, .sampler = &ssao_sampler
    }};
    render_pass.bind_fragment_samplers(ssao_sampler_slot, ssao_sampler_bindings);

    for (const auto& batch : instance_batches) {
        const auto& instance_buffer =
            instance_buffer_cache.get(batch.renderable_id);
        const auto storage_buffer_bindings = std::array{&instance_buffer};
        render_pass.bind_vertex_storage_buffers(0, storage_buffer_bindings);

        for (const auto& mesh :
             graphics_factory.get_meshes(batch.renderable_id)) {
            mesh.draw_instanced(
                static_cast<int32_t>(batch.instance_count), render_pass
            );
        }
    }
}

}  // namespace Luminol::Graphics::SDL_GPU
