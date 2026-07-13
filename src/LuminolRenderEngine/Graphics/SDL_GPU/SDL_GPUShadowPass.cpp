#include "SDL_GPUShadowPass.hpp"

#include <algorithm>
#include <array>
#include <cmath>

#include <LuminolMaths/Transform.hpp>

#include <LuminolRenderEngine/Graphics/Frustum.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCommandBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCullingUtils.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUDevice.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUFactory.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPURenderPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUResourceBuilders.hpp>
#include <LuminolRenderEngine/Utilities/Timer.hpp>

namespace {

using namespace Luminol::Graphics::SDL_GPU;
using namespace Luminol::Graphics;
using namespace Luminol::Maths;

constexpr auto shadow_map_format = TextureFormat::D24_Unorm;

constexpr auto shadow_map_resolution = 2048U;
constexpr auto shadow_near_plane = 0.1F;

// Cascades only need to cover shadows out to this distance from the camera,
// not the camera's true far plane (which may be far beyond where shadows
// are visually relevant) - keeps the far cascades from being wasted on
// distant geometry.
constexpr auto max_shadow_distance = 100.0F;

// Blend factor between a uniform and logarithmic cascade split scheme (the
// "practical split scheme"): 0 = uniform, 1 = fully logarithmic. Biased
// heavily toward logarithmic so cascade 0 stays small (sharp, high texel
// density close to the camera) instead of ballooning out to roughly a third
// of max_shadow_distance the way a 0.5 blend does - a pure uniform split
// wastes the near cascade's resolution on distant geometry.
constexpr auto cascade_split_lambda = 0.85F;

// Extra distance the light is pulled back behind each cascade's bounding
// sphere so that casters just outside the visible frustum slice (e.g.
// behind the camera or off to the side) still end up in the depth range
// and aren't missing from the shadow map. Decoupled from the per-cascade
// ortho half-extent, mirroring how the original single-cascade shadow pass
// used a separate shadow_distance from shadow_ortho_half_extent.
constexpr auto caster_padding = max_shadow_distance;

constexpr auto world_up = Vector3f{0.0F, 1.0F, 0.0F};
constexpr auto up_fallback = Vector3f{0.0F, 0.0F, 1.0F};

// Row-major, left-handed orthographic projection with a [0, 1] depth range,
// matching the D3D-style convention used by
// Transform::left_handed_perspective_projection_matrix.
// Mirrors cbuffer UBO in pbr_vert.hlsl.
struct VertexUBO {
    Matrix4x4f light_space_matrix;
    std::array<uint32_t, 4> instance_base_offset;
};

auto left_handed_orthographic_projection_matrix(
    float half_width, float half_height, float near_plane, float far_plane
) -> Matrix4x4f {
    auto result = Matrix4x4f::zero();

    result[0][0] = 1.0F / half_width;
    result[1][1] = 1.0F / half_height;
    result[2][2] = 1.0F / (far_plane - near_plane);
    result[3][2] = -near_plane / (far_plane - near_plane);
    result[3][3] = 1.0F;

    return result;
}

// Extracts the camera's near/far plane distances directly from its
// projection matrix (see Maths::Transform::left_handed_perspective_projection_matrix),
// so the shadow pass doesn't need a separate camera-params API - it only
// ever receives the already-computed view/projection matrices, mirroring
// extract_camera_params in SDL_GPURenderer.cpp.
struct CameraNearFar {
    float near_plane;
    float far_plane;
};

auto extract_camera_near_far(const Matrix4x4f& projection_matrix)
    -> CameraNearFar {
    const auto c = projection_matrix[2][2];
    const auto e = projection_matrix[3][2];

    return CameraNearFar{
        .near_plane = -e / c,
        .far_plane = e / (1.0F - c),
    };
}

// Builds the perspective projection covering just [split_near, split_far]
// of the camera's frustum, reusing the camera's existing fov/aspect terms
// (rows 0 and 1, which projection_matrix already encodes) and only
// recomputing the near/far-dependent terms (see
// Transform::left_handed_perspective_projection_matrix's C/E derivation).
auto make_sub_frustum_projection(
    const Matrix4x4f& projection_matrix, float split_near, float split_far
) -> Matrix4x4f {
    auto result = projection_matrix;
    const auto range = split_far - split_near;

    result[2][2] = split_far / range;
    result[3][2] = -split_near * split_far / range;

    return result;
}

// Practical split scheme: blends a uniform split and a logarithmic split of
// [camera_near, effective_far] by cascade_split_lambda. Returns
// shadow_pass_num_cascades + 1 boundaries; cascade i covers
// [splits[i], splits[i + 1]].
auto compute_cascade_splits(float camera_near, float camera_far)
    -> std::array<float, shadow_pass_num_cascades + 1> {
    const auto effective_far = std::min(camera_far, max_shadow_distance);

    auto splits = std::array<float, shadow_pass_num_cascades + 1>{};
    splits[0] = camera_near;

    for (auto i = 1U; i <= shadow_pass_num_cascades; ++i) {
        const auto t =
            static_cast<float>(i) / static_cast<float>(shadow_pass_num_cascades);

        const auto log_split =
            camera_near * std::pow(effective_far / camera_near, t);
        const auto uniform_split =
            camera_near + (effective_far - camera_near) * t;

        splits.at(i) = (cascade_split_lambda * log_split) +
            ((1.0F - cascade_split_lambda) * uniform_split);
    }

    return splits;
}

// Fits an orthographic light-space matrix around the world-space frustum
// slice [split_near, split_far], using a bounding sphere (rather than a
// tight AABB) so the box's world-space size stays constant regardless of
// camera yaw/pitch - this is what prevents shadow-edge shimmer as the
// camera turns. Texel-snaps the sphere center along the light's right/up
// axes (same trick the original single-cascade pass used) to keep any
// residual movement to whole-texel steps.
auto compute_cascade_light_space_matrix(
    const Vector3f& light_direction,
    const Matrix4x4f& view_matrix,
    const Matrix4x4f& projection_matrix,
    float split_near,
    float split_far
) -> Matrix4x4f {
    const auto direction = light_direction.normalized();
    const auto light_up_vector =
        std::abs(direction.dot(world_up)) > 0.99F ? up_fallback : world_up;

    const auto sub_projection =
        make_sub_frustum_projection(projection_matrix, split_near, split_far);
    const auto corners =
        extract_frustum_corners(view_matrix * sub_projection);

    auto center = Vector3f{0.0F, 0.0F, 0.0F};
    for (const auto& corner : corners) {
        center = center + corner;
    }
    center = center * (1.0F / static_cast<float>(corners.size()));

    auto radius = 0.0F;
    for (const auto& corner : corners) {
        radius = std::max(radius, (corner - center).length());
    }

    const auto right = light_up_vector.cross(direction).normalized();
    const auto up_axis = direction.cross(right);

    const auto texel_size = (2.0F * radius) / static_cast<float>(shadow_map_resolution);

    const auto snap = [texel_size](float value) {
        return std::floor(value / texel_size) * texel_size;
    };

    const auto center_right_dist = right.dot(center);
    const auto center_up_dist = up_axis.dot(center);

    const auto snapped_center = center +
        right * (snap(center_right_dist) - center_right_dist) +
        up_axis * (snap(center_up_dist) - center_up_dist);

    const auto light_eye =
        snapped_center - direction * (radius + caster_padding);

    const auto light_view = Transform::left_handed_look_at_matrix(
        Transform::LookAtParams<float>{
            .eye = light_eye,
            .target = snapped_center,
            .up_vector = light_up_vector
        }
    );

    const auto light_projection = left_handed_orthographic_projection_matrix(
        radius, radius, shadow_near_plane, (2.0F * radius) + caster_padding
    );

    return light_view * light_projection;
}

auto make_shadow_map_texture(GPUDevice& device) -> Texture {
    return device.create_texture(TextureInfo{
        .width = shadow_map_resolution,
        .height = shadow_map_resolution,
        .format = shadow_map_format,
        .usage = TextureUsage::DepthStencilTarget | TextureUsage::Sampler,
        .type = TextureType::Texture2DArray,
        .layer_count = shadow_pass_num_cascades,
    });
}

}  // namespace

namespace Luminol::Graphics::SDL_GPU {

SDL_GPUShadowPass::SDL_GPUShadowPass(GPUDevice& device)
    : shadow_vertex_shader{make_hlsl_shader(
          device, "res/shaders/sdl_gpu/pbr_vert.hlsl", ShaderStage::Vertex,
          0U, 1U, 2U
      )},
      shadow_fragment_shader{make_hlsl_shader(
          device, "res/shaders/sdl_gpu/shadow_depth_frag.hlsl",
          ShaderStage::Fragment
      )},
      shadow_pipeline{make_depth_only_mesh_pipeline(
          device, shadow_vertex_shader, shadow_fragment_shader,
          shadow_map_format
      )},
      shadow_map_texture{make_shadow_map_texture(device)},
      shadow_map_sampler{device.create_sampler(SamplerInfo{
          .filter = SamplerFilter::Linear,
          .address_mode_u = SamplerAddressMode::ClampToEdge,
          .address_mode_v = SamplerAddressMode::ClampToEdge,
          .enable_compare = true,
      })},
      cascade_cull_passes{
          SDL_GPUInstanceCullPass{device}, SDL_GPUInstanceCullPass{device},
          SDL_GPUInstanceCullPass{device}, SDL_GPUInstanceCullPass{device}
      },
      cascade_light_space_matrices{
          Matrix4x4f::identity(), Matrix4x4f::identity(),
          Matrix4x4f::identity(), Matrix4x4f::identity()
      } {}

auto SDL_GPUShadowPass::draw(
    const SDL_GPUFactory& graphics_factory,
    CommandBuffer& command_buffer,
    const SDL_GPUInstanceBufferCache& instance_buffer_cache,
    gsl::span<const InstanceBatch> instance_batches,
    const std::unordered_map<RenderableId, std::vector<Maths::Matrix4x4f>>&
        queued_draws,
    const Maths::Vector3f& light_direction,
    const Maths::Matrix4x4f& view_matrix,
    const Maths::Matrix4x4f& projection_matrix,
    const Texture& hiz_pyramid,
    const Sampler& hiz_sampler,
    Utilities::PerformanceLogger& performance_logger
) -> void {
    const auto pass_timer = Utilities::Timer{};
    command_buffer.push_debug_group("shadow_pass");

    const auto camera_near_far = extract_camera_near_far(projection_matrix);
    const auto splits = compute_cascade_splits(
        camera_near_far.near_plane, camera_near_far.far_plane
    );

    // One world-space AABB per (batch, submesh), reused for every cascade's
    // frustum test below instead of being recomputed per cascade.
    const auto batch_mesh_world_bounds = compute_batch_mesh_world_bounds(
        graphics_factory, instance_batches, queued_draws
    );

    const auto shadow_map_texture_view =
        TextureView{shadow_map_texture.native_handle()};

    // Phase 1: per-cascade GPU instance culling. Must all happen before any
    // render pass is opened below - SDL_GPU forbids opening a compute/copy
    // pass while a render pass is active, so every cascade's cull() (which
    // opens its own copy + compute passes) runs first, and its results
    // (filtered batch list + resulting layout) are stashed for phase 2.
    auto cascade_filtered_batches =
        std::array<std::vector<InstanceBatch>, shadow_pass_num_cascades>{};
    auto cascade_cull_layouts =
        std::array<InstanceCullLayout, shadow_pass_num_cascades>{};

    for (auto cascade_index = 0U; cascade_index < shadow_pass_num_cascades;
         ++cascade_index) {
        const auto split_near = splits.at(cascade_index);
        const auto split_far = splits.at(cascade_index + 1);

        cascade_light_space_matrices.at(cascade_index) =
            compute_cascade_light_space_matrix(
                light_direction, view_matrix, projection_matrix, split_near,
                split_far
            );
        cascade_split_depths[cascade_index] = split_far;

        const auto cascade_frustum_planes = extract_frustum_planes(
            cascade_light_space_matrices.at(cascade_index)
        );

        // Coarse pre-filter: skip this batch entirely for this cascade if
        // none of its submeshes' world-space bounds intersect the cascade's
        // orthographic frustum, so the per-instance GPU cull dispatch below
        // isn't even issued for batches nowhere near this cascade.
        auto& filtered_batches = cascade_filtered_batches[cascade_index];
        for (auto batch_index = std::size_t{0};
             batch_index < instance_batches.size(); ++batch_index) {
            const auto& batch = instance_batches[batch_index];
            const auto& mesh_bounds = batch_mesh_world_bounds[batch_index];
            const auto batch_in_frustum = std::any_of(
                mesh_bounds.begin(), mesh_bounds.end(),
                [&cascade_frustum_planes](const BoundingBox& bounds) {
                    return aabb_in_frustum(
                        cascade_frustum_planes, bounds.min, bounds.max
                    );
                }
            );
            if (batch_in_frustum) {
                filtered_batches.push_back(batch);
            }
        }

        cascade_cull_layouts[cascade_index] =
            cascade_cull_passes[cascade_index].cull(
                graphics_factory, command_buffer, instance_buffer_cache,
                filtered_batches, cascade_frustum_planes,
                cascade_light_space_matrices.at(cascade_index), hiz_pyramid,
                hiz_sampler, 0U
            );
    }

    // Phase 2: per-cascade render passes, drawing only the instances each
    // cascade's cull pass compacted into visible_instance_indices.
    for (auto cascade_index = 0U; cascade_index < shadow_pass_num_cascades;
         ++cascade_index) {
        const auto& filtered_batches = cascade_filtered_batches[cascade_index];
        const auto& cull_layout = cascade_cull_layouts[cascade_index];
        const auto& cull_pass = cascade_cull_passes[cascade_index];

        const auto depth_stencil_target = DepthStencilTargetInfo{
            .texture = &shadow_map_texture_view,
            .clear_depth = 1.0F,
            .load_op = LoadOp::Clear,
            .store_op = StoreOp::Store,
            .cycle = cascade_index == 0,
            .layer = cascade_index,
        };

        auto render_pass =
            command_buffer.begin_render_pass({}, &depth_stencil_target);
        render_pass.bind_graphics_pipeline(shadow_pipeline);

        for (auto batch_index = std::size_t{0};
             batch_index < filtered_batches.size(); ++batch_index) {
            const auto& batch = filtered_batches[batch_index];

            const auto& instance_buffer =
                instance_buffer_cache.get(batch.renderable_id);
            const auto& visible_instance_indices_buffer =
                cull_pass.get_visible_instance_indices_buffer();
            const auto storage_buffer_bindings = std::array{
                &instance_buffer, &visible_instance_indices_buffer
            };
            render_pass.bind_vertex_storage_buffers(
                0, storage_buffer_bindings
            );

            const auto vertex_bindings = std::array{VertexBufferBinding{
                .buffer =
                    &graphics_factory.get_vertex_buffer(batch.renderable_id),
                .offset = 0,
            }};
            render_pass.bind_vertex_buffers(0, vertex_bindings);
            render_pass.bind_index_buffer(
                graphics_factory.get_index_buffer(batch.renderable_id),
                IndexElementSize::Bits32, 0
            );

            const auto& submesh_infos = cull_layout[batch_index];
            const auto meshes = graphics_factory.get_meshes(batch.renderable_id);
            for (auto mesh_index = std::size_t{0}; mesh_index < meshes.size();
                 ++mesh_index) {
                const auto& submesh_info = submesh_infos[mesh_index];

                const auto vertex_ubo = VertexUBO{
                    .light_space_matrix =
                        cascade_light_space_matrices.at(cascade_index),
                    .instance_base_offset =
                        {submesh_info.instance_base_offset, 0, 0, 0},
                };
                command_buffer.push_vertex_uniform_data(
                    0,
                    gsl::span{
                        reinterpret_cast<const std::byte*>(&vertex_ubo),
                        sizeof(vertex_ubo)
                    }
                );

                meshes[mesh_index].draw_indirect_geometry_only(
                    render_pass, cull_pass.get_indirect_command_buffer(),
                    submesh_info.indirect_command_byte_offset
                );
            }
        }
    }

    command_buffer.pop_debug_group();
    performance_logger.record(
        "shadow_pass", Units::Seconds{pass_timer.elapsed_seconds()}
    );
}

auto SDL_GPUShadowPass::get_shadow_map_texture() const -> const Texture& {
    return shadow_map_texture;
}

auto SDL_GPUShadowPass::get_sampler() const -> const Sampler& {
    return shadow_map_sampler;
}

auto SDL_GPUShadowPass::get_cascade_light_space_matrices() const
    -> const std::array<Maths::Matrix4x4f, shadow_pass_num_cascades>& {
    return cascade_light_space_matrices;
}

auto SDL_GPUShadowPass::get_cascade_split_depths() const
    -> const Maths::Vector4f& {
    return cascade_split_depths;
}

}  // namespace Luminol::Graphics::SDL_GPU
