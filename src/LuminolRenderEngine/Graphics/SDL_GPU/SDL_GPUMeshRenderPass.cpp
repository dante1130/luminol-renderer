#include "SDL_GPUMeshRenderPass.hpp"

#include <algorithm>
#include <array>
#include <optional>

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

constexpr auto fragment_sampler_count = 12U;
constexpr auto ssao_sampler_slot = 5U;
constexpr auto shadow_map_sampler_slot = 6U;
constexpr auto irradiance_sampler_slot = 7U;
constexpr auto prefiltered_sampler_slot = 8U;
constexpr auto brdf_lut_sampler_slot = 9U;
constexpr auto point_shadow_sampler_slot = 10U;
constexpr auto spot_shadow_sampler_slot = 11U;

// Clustered Forward+ light buffers, bound as fragment storage buffers
// (t10-t13, space2 - continuing the t-register index right after the 10
// textures/samplers above, per SDL_GPU's fragment resource-space
// convention).
constexpr auto cluster_light_buffer_count = 4U;
constexpr auto cluster_light_buffer_slot = 0U;

// Spot shadow-matrix buffer (t16, space2), the fifth and last fragment
// storage buffer.
constexpr auto spot_shadow_matrix_buffer_slot = cluster_light_buffer_count;
constexpr auto fragment_storage_buffer_count = cluster_light_buffer_count + 1U;

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
        .storage_buffer_count = stage == ShaderStage::Vertex ? 1U
            : stage == ShaderStage::Fragment  ? fragment_storage_buffer_count
                                               : 0U,
    });
}

struct TransparentDrawItem {
    Luminol::Graphics::RenderableId renderable_id;
    const SDL_GPUMesh* mesh;
    uint32_t instance_count;
    float distance_squared;
};

// Approximates a transparent batch's depth by the squared distance from the
// camera to the average translation of its instances. This sorts distinct
// transparent renderables back-to-front, but does not order individual
// instances within a single instanced draw of the same renderable.
auto batch_distance_squared_to_camera(
    gsl::span<const Luminol::Maths::Matrix4x4f> model_matrices,
    const Luminol::Maths::Vector4f& camera_position
) -> float {
    auto sum_x = 0.0F;
    auto sum_y = 0.0F;
    auto sum_z = 0.0F;
    for (const auto& model_matrix : model_matrices) {
        sum_x += model_matrix[3][0];
        sum_y += model_matrix[3][1];
        sum_z += model_matrix[3][2];
    }

    const auto instance_count = static_cast<float>(model_matrices.size());
    const auto diff_x = (sum_x / instance_count) - camera_position.x();
    const auto diff_y = (sum_y / instance_count) - camera_position.y();
    const auto diff_z = (sum_z / instance_count) - camera_position.z();

    return (diff_x * diff_x) + (diff_y * diff_y) + (diff_z * diff_z);
}

auto make_mesh_pipeline(
    GPUDevice& device,
    const Shader& vertex_shader,
    const Shader& fragment_shader,
    SampleCount sample_count
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
        .sample_count = sample_count,
    });
}

// Blended, depth-tested-but-not-depth-written variant of the mesh pipeline
// for transparent meshes. Culling is disabled so both faces of thin
// transparent geometry (e.g. glass panes, foliage cards) are visible.
auto make_mesh_transparent_pipeline(
    GPUDevice& device,
    const Shader& vertex_shader,
    const Shader& fragment_shader,
    SampleCount sample_count
) -> GraphicsPipeline {
    return device.create_graphics_pipeline(GraphicsPipelineInfo{
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_target_format = hdr_color_texture_format,
        .primitive_type = PrimitiveType::TriangleList,
        .vertex_buffer_descriptions = mesh_vertex_buffer_descriptions,
        .vertex_attributes = mesh_vertex_attributes,
        .enable_depth_test = true,
        .enable_depth_write = false,
        .depth_stencil_format = depth_texture_format,
        .cull_mode = CullMode::None,
        .front_face = FrontFace::Clockwise,
        .enable_blend = true,
        .sample_count = sample_count,
    });
}

// Opaque-style depth test/write, no blending, but using the alpha-test
// fragment shader (discards below the glTF-default 0.5 cutoff). Culling is
// disabled to match the doubleSided flag on the alpha-tested materials seen
// so far (foliage/thorn/chain cutout geometry).
auto make_mesh_alpha_test_pipeline(
    GPUDevice& device,
    const Shader& vertex_shader,
    const Shader& fragment_shader,
    SampleCount sample_count
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
        .cull_mode = CullMode::None,
        .front_face = FrontFace::Clockwise,
        .sample_count = sample_count,
    });
}

}  // namespace

namespace Luminol::Graphics::SDL_GPU {

SDL_GPUMeshRenderPass::SDL_GPUMeshRenderPass(
    GPUDevice& device, SampleCount sample_count
)
    : mesh_vertex_shader{make_mesh_shader(
          device, "res/shaders/sdl_gpu/pbr_vert.hlsl", ShaderStage::Vertex
      )},
      mesh_fragment_shader{make_mesh_shader(
          device, "res/shaders/sdl_gpu/pbr_frag.hlsl", ShaderStage::Fragment
      )},
      mesh_alpha_test_fragment_shader{make_mesh_shader(
          device, "res/shaders/sdl_gpu/pbr_frag_alpha_test.hlsl",
          ShaderStage::Fragment
      )},
      mesh_pipeline{make_mesh_pipeline(
          device, mesh_vertex_shader, mesh_fragment_shader, sample_count
      )},
      mesh_alpha_test_pipeline{make_mesh_alpha_test_pipeline(
          device, mesh_vertex_shader, mesh_alpha_test_fragment_shader,
          sample_count
      )},
      mesh_transparent_pipeline{make_mesh_transparent_pipeline(
          device, mesh_vertex_shader, mesh_fragment_shader, sample_count
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
    const std::unordered_map<RenderableId, std::vector<Maths::Matrix4x4f>>&
        queued_draws,
    const Maths::Matrix4x4f& view_proj,
    LightData light_data,
    const Texture& ssao_texture,
    const Sampler& ssao_sampler,
    const Texture& shadow_map_texture,
    const Sampler& shadow_map_sampler,
    const IBLTextures& ibl_textures,
    const ClusteredLightBuffers& clustered_light_buffers,
    const PointSpotShadowTextures& point_spot_shadow_textures
) -> void {
    light_data.shadow_params.z() =
        static_cast<float>(ibl_textures.prefiltered_mip_count - 1);

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

    const auto cluster_light_buffer_bindings = std::array<const Buffer* const, 4>{
        clustered_light_buffers.point_lights,
        clustered_light_buffers.spot_lights,
        clustered_light_buffers.cluster_light_grid,
        clustered_light_buffers.global_light_index_list,
    };
    render_pass.bind_fragment_storage_buffers(
        cluster_light_buffer_slot, cluster_light_buffer_bindings
    );

    const auto ssao_sampler_bindings = std::array{TextureSamplerBinding{
        .texture = &ssao_texture, .sampler = &ssao_sampler
    }};
    render_pass.bind_fragment_samplers(ssao_sampler_slot, ssao_sampler_bindings);

    const auto shadow_map_sampler_bindings = std::array{TextureSamplerBinding{
        .texture = &shadow_map_texture, .sampler = &shadow_map_sampler
    }};
    render_pass.bind_fragment_samplers(
        shadow_map_sampler_slot, shadow_map_sampler_bindings
    );

    const auto irradiance_sampler_bindings = std::array{TextureSamplerBinding{
        .texture = ibl_textures.irradiance_texture,
        .sampler = ibl_textures.irradiance_sampler
    }};
    render_pass.bind_fragment_samplers(
        irradiance_sampler_slot, irradiance_sampler_bindings
    );

    const auto prefiltered_sampler_bindings = std::array{TextureSamplerBinding{
        .texture = ibl_textures.prefiltered_texture,
        .sampler = ibl_textures.prefiltered_sampler
    }};
    render_pass.bind_fragment_samplers(
        prefiltered_sampler_slot, prefiltered_sampler_bindings
    );

    const auto brdf_lut_sampler_bindings = std::array{TextureSamplerBinding{
        .texture = ibl_textures.brdf_lut_texture,
        .sampler = ibl_textures.brdf_lut_sampler
    }};
    render_pass.bind_fragment_samplers(
        brdf_lut_sampler_slot, brdf_lut_sampler_bindings
    );

    const auto point_shadow_sampler_bindings = std::array{TextureSamplerBinding{
        .texture = point_spot_shadow_textures.point_shadow_texture,
        .sampler = point_spot_shadow_textures.point_shadow_sampler
    }};
    render_pass.bind_fragment_samplers(
        point_shadow_sampler_slot, point_shadow_sampler_bindings
    );

    const auto spot_shadow_sampler_bindings = std::array{TextureSamplerBinding{
        .texture = point_spot_shadow_textures.spot_shadow_texture,
        .sampler = point_spot_shadow_textures.spot_shadow_sampler
    }};
    render_pass.bind_fragment_samplers(
        spot_shadow_sampler_slot, spot_shadow_sampler_bindings
    );

    const auto spot_shadow_matrix_buffer_bindings =
        std::array<const Buffer* const, 1>{
            point_spot_shadow_textures.spot_shadow_matrices
        };
    render_pass.bind_fragment_storage_buffers(
        spot_shadow_matrix_buffer_slot, spot_shadow_matrix_buffer_bindings
    );

    const auto draw_batches_matching =
        [&](Utilities::ModelLoader::AlphaMode mode) {
            for (const auto& batch : instance_batches) {
                const auto& instance_buffer =
                    instance_buffer_cache.get(batch.renderable_id);
                const auto storage_buffer_bindings =
                    std::array{&instance_buffer};
                render_pass.bind_vertex_storage_buffers(
                    0, storage_buffer_bindings
                );

                for (const auto& mesh :
                     graphics_factory.get_meshes(batch.renderable_id)) {
                    if (mesh.alpha_mode() != mode) {
                        continue;
                    }

                    mesh.draw_instanced(
                        static_cast<int32_t>(batch.instance_count), render_pass
                    );
                }
            }
        };

    render_pass.bind_graphics_pipeline(mesh_pipeline);
    draw_batches_matching(Utilities::ModelLoader::AlphaMode::Opaque);

    render_pass.bind_graphics_pipeline(mesh_alpha_test_pipeline);
    draw_batches_matching(Utilities::ModelLoader::AlphaMode::Mask);

    auto transparent_items = std::vector<TransparentDrawItem>{};

    for (const auto& batch : instance_batches) {
        const auto meshes = graphics_factory.get_meshes(batch.renderable_id);
        const auto has_transparent_mesh = std::ranges::any_of(
            meshes,
            [](const auto& mesh) {
                return mesh.alpha_mode() ==
                    Utilities::ModelLoader::AlphaMode::Blend;
            }
        );
        if (!has_transparent_mesh) {
            continue;
        }

        const auto& model_matrices = queued_draws.at(batch.renderable_id);
        const auto distance_squared = batch_distance_squared_to_camera(
            model_matrices, light_data.view_position
        );

        for (const auto& mesh : meshes) {
            if (mesh.alpha_mode() != Utilities::ModelLoader::AlphaMode::Blend) {
                continue;
            }

            transparent_items.push_back(TransparentDrawItem{
                .renderable_id = batch.renderable_id,
                .mesh = &mesh,
                .instance_count = batch.instance_count,
                .distance_squared = distance_squared,
            });
        }
    }

    if (transparent_items.empty()) {
        return;
    }

    // Farthest first, so nearer transparent surfaces blend on top of ones
    // behind them.
    std::ranges::sort(
        transparent_items,
        [](const TransparentDrawItem& lhs, const TransparentDrawItem& rhs) {
            return lhs.distance_squared > rhs.distance_squared;
        }
    );

    render_pass.bind_graphics_pipeline(mesh_transparent_pipeline);

    auto bound_renderable_id = std::optional<RenderableId>{};
    for (const auto& item : transparent_items) {
        if (bound_renderable_id != item.renderable_id) {
            const auto& instance_buffer =
                instance_buffer_cache.get(item.renderable_id);
            const auto storage_buffer_bindings = std::array{&instance_buffer};
            render_pass.bind_vertex_storage_buffers(0, storage_buffer_bindings);
            bound_renderable_id = item.renderable_id;
        }

        item.mesh->draw_instanced(
            static_cast<int32_t>(item.instance_count), render_pass
        );
    }
}

}  // namespace Luminol::Graphics::SDL_GPU
