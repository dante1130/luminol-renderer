#include "SDL_GPUPointSpotShadowPass.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <numbers>
#include <unordered_map>
#include <vector>

#include <LuminolMaths/Transform.hpp>
#include <LuminolMaths/Units/Angle.hpp>

#include <LuminolRenderEngine/Graphics/BoundingBox.hpp>
#include <LuminolRenderEngine/Graphics/Frustum.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCommandBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCopyPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCullingUtils.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUDevice.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUFactory.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPURenderPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUResourceBuilders.hpp>
#include <LuminolRenderEngine/Utilities/Timer.hpp>

namespace {

using namespace Luminol::Graphics;
using namespace Luminol::Graphics::SDL_GPU;
using namespace Luminol::Maths;

constexpr auto shadow_map_format = TextureFormat::D24_Unorm;

constexpr auto point_shadow_resolution = 512U;
constexpr auto spot_shadow_resolution = 1024U;
constexpr auto point_shadow_near_plane = 0.05F;
constexpr auto spot_shadow_near_plane = 0.05F;
constexpr auto cube_faces_per_light = 6U;

// Atlas grid layout: every point-light cube face / spot light gets a fixed
// tile in one shared 2D texture, addressed by (slot, face) rather than an
// array layer. Grid dims are hardcoded to exactly fit the shadow-caster caps
// (see static_asserts below) - must match the equivalent constants in
// pbr_frag.hlsl.
constexpr auto point_atlas_cols = 8U;
constexpr auto point_atlas_rows = 12U;
constexpr auto spot_atlas_cols = 8U;
constexpr auto spot_atlas_rows = 4U;

static_assert(
    point_atlas_cols * point_atlas_rows >=
    Luminol::Graphics::max_shadow_casting_point_lights * cube_faces_per_light
);
static_assert(
    spot_atlas_cols * spot_atlas_rows >=
    Luminol::Graphics::max_shadow_casting_spot_lights
);

constexpr auto point_atlas_width = point_atlas_cols * point_shadow_resolution;
constexpr auto point_atlas_height = point_atlas_rows * point_shadow_resolution;
constexpr auto spot_atlas_width = spot_atlas_cols * spot_shadow_resolution;
constexpr auto spot_atlas_height = spot_atlas_rows * spot_shadow_resolution;

// Mirrors cbuffer UBO in pbr_vert.hlsl.
struct VertexUBO {
    Matrix4x4f view_projection;
    std::array<uint32_t, 4> instance_base_offset;
};

struct AtlasTileRect {
    uint32_t x;
    uint32_t y;
    uint32_t size;
};

auto point_atlas_tile_rect(uint32_t slot, uint32_t face) -> AtlasTileRect {
    const auto tile_index = (slot * cube_faces_per_light) + face;
    return AtlasTileRect{
        .x = (tile_index % point_atlas_cols) * point_shadow_resolution,
        .y = (tile_index / point_atlas_cols) * point_shadow_resolution,
        .size = point_shadow_resolution,
    };
}

auto spot_atlas_tile_rect(uint32_t slot) -> AtlasTileRect {
    return AtlasTileRect{
        .x = (slot % spot_atlas_cols) * spot_shadow_resolution,
        .y = (slot / spot_atlas_cols) * spot_shadow_resolution,
        .size = spot_shadow_resolution,
    };
}

constexpr auto spot_shadow_matrix_buffer_size =
    Luminol::Graphics::max_shadow_casting_spot_lights *
    static_cast<uint32_t>(sizeof(Matrix4x4f));

// Generous upper bound on (tile, batch) draw commands in a single frame:
// (max_shadow_casting_point_lights * 6 faces + max_shadow_casting_spot_lights)
// tiles, times an assumed-generous max of distinct renderable batches drawn
// in one frame. Actual per-frame usage is almost always far smaller (most
// scenes have a handful of distinct renderables); this only needs to be a
// safe ceiling, not a tight estimate.
constexpr auto assumed_max_batches_per_frame = 64U;
constexpr auto max_indirect_draw_commands =
    (Luminol::Graphics::max_shadow_casting_point_lights * cube_faces_per_light +
     Luminol::Graphics::max_shadow_casting_spot_lights) *
    assumed_max_batches_per_frame;
constexpr auto indirect_draw_buffer_size =
    max_indirect_draw_commands * static_cast<uint32_t>(sizeof(IndirectDrawCommand));

struct CubeFace {
    Vector3f target_offset;
    Vector3f up;
};

// Standard cubemap face capture basis, matching the +X,-X,+Y,-Y,+Z,-Z face
// order already used for the skybox/IBL cubemap faces (SDL_GPUSkybox.cpp,
// SDL_GPUIBLRenderPass.cpp). SDL_GPU addresses a cube array's physical
// layers as 6 * array_index + face_index, matching this order.
constexpr auto cube_faces = std::array{
    CubeFace{.target_offset = Vector3f{1.0F, 0.0F, 0.0F}, .up = Vector3f{0.0F, 1.0F, 0.0F}},
    CubeFace{.target_offset = Vector3f{-1.0F, 0.0F, 0.0F}, .up = Vector3f{0.0F, 1.0F, 0.0F}},
    CubeFace{.target_offset = Vector3f{0.0F, 1.0F, 0.0F}, .up = Vector3f{0.0F, 0.0F, -1.0F}},
    CubeFace{.target_offset = Vector3f{0.0F, -1.0F, 0.0F}, .up = Vector3f{0.0F, 0.0F, 1.0F}},
    CubeFace{.target_offset = Vector3f{0.0F, 0.0F, 1.0F}, .up = Vector3f{0.0F, 1.0F, 0.0F}},
    CubeFace{.target_offset = Vector3f{0.0F, 0.0F, -1.0F}, .up = Vector3f{0.0F, 1.0F, 0.0F}},
};

// Must match the light_cull_radius cutoff in pbr_frag.hlsl /
// cluster_light_count.hlsl / cluster_light_compact.hlsl / LightManager.cpp
// exactly, so a shadow's far plane matches the sphere shading fades the
// light's contribution to zero at.
auto light_cull_radius(const Vector3f& color) -> float {
    constexpr auto cutoff = 1.0F / 16.0F;
    const auto intensity = std::max({color.x(), color.y(), color.z()});
    return std::sqrt(std::max(intensity, 0.0F) / cutoff);
}

// A dim light's light_cull_radius can come out smaller than (or too close
// to) the near plane, which would make the perspective projection's
// far/(far-near) terms blow up into NaN/Inf - poisoning the depth buffer
// and, on some drivers, the GPU itself (observed as VK_ERROR_DEVICE_LOST).
// Keep a minimum spread between the two.
constexpr auto min_near_far_spread = 0.1F;

auto point_light_face_view_projection(
    const Vector3f& position, float far_plane, uint32_t face
) -> Matrix4x4f {
    const auto& face_info = gsl::at(cube_faces, face);

    const auto view = Transform::left_handed_look_at_matrix(
        Transform::LookAtParams<float>{
            .eye = position,
            .target = position + face_info.target_offset,
            .up_vector = face_info.up,
        }
    );

    const auto safe_far_plane =
        std::max(far_plane, point_shadow_near_plane + min_near_far_spread);

    const auto projection = Transform::left_handed_perspective_projection_matrix(
        Transform::PerspectiveMatrixParams<float>{
            .fov = Luminol::Units::Degrees_f{90.0F},
            .aspect_ratio = 1.0F,
            .near_plane = point_shadow_near_plane,
            .far_plane = safe_far_plane,
        }
    );

    return view * projection;
}

constexpr auto world_up = Vector3f{0.0F, 1.0F, 0.0F};
constexpr auto up_fallback = Vector3f{0.0F, 0.0F, 1.0F};

auto spot_light_view_projection(
    const Vector3f& position,
    const Vector3f& direction,
    float outer_cut_off,
    float far_plane
) -> Matrix4x4f {
    const auto forward = direction.normalized();
    const auto light_up_vector =
        std::abs(forward.dot(world_up)) > 0.99F ? up_fallback : world_up;

    const auto view = Transform::left_handed_look_at_matrix(
        Transform::LookAtParams<float>{
            .eye = position,
            .target = position + forward,
            .up_vector = light_up_vector,
        }
    );

    // outer_cut_off is stored as cos(half-angle) (see calculate_spot_light in
    // pbr_frag.hlsl); clamp away from +-1 so the derived fov never
    // approaches 180 degrees, which would make tan(fov/2) blow up.
    const auto clamped_outer_cut_off =
        std::clamp(outer_cut_off, -0.999F, 0.999F);
    const auto fov_degrees = std::min(
        2.0F * std::acos(clamped_outer_cut_off) * (180.0F / std::numbers::pi_v<float>),
        170.0F
    );

    const auto safe_far_plane =
        std::max(far_plane, spot_shadow_near_plane + min_near_far_spread);

    const auto projection = Transform::left_handed_perspective_projection_matrix(
        Transform::PerspectiveMatrixParams<float>{
            .fov = Luminol::Units::Degrees_f{fov_degrees},
            .aspect_ratio = 1.0F,
            .near_plane = spot_shadow_near_plane,
            .far_plane = safe_far_plane,
        }
    );

    return view * projection;
}

auto make_point_shadow_texture(GPUDevice& device) -> Texture {
    return device.create_texture(TextureInfo{
        .width = point_atlas_width,
        .height = point_atlas_height,
        .format = shadow_map_format,
        .usage = TextureUsage::DepthStencilTarget | TextureUsage::Sampler,
        .type = TextureType::Texture2D,
    });
}

auto make_spot_shadow_texture(GPUDevice& device) -> Texture {
    return device.create_texture(TextureInfo{
        .width = spot_atlas_width,
        .height = spot_atlas_height,
        .format = shadow_map_format,
        .usage = TextureUsage::DepthStencilTarget | TextureUsage::Sampler,
        .type = TextureType::Texture2D,
    });
}

auto make_shadow_map_sampler(GPUDevice& device) -> Sampler {
    return device.create_sampler(SamplerInfo{
        .filter = SamplerFilter::Linear,
        .address_mode_u = SamplerAddressMode::ClampToEdge,
        .address_mode_v = SamplerAddressMode::ClampToEdge,
        .enable_compare = true,
    });
}

struct SelectedPointLight {
    uint32_t slot;
    Vector3f position;
    float far_plane;
};

struct SelectedSpotLight {
    uint32_t slot;
    Vector3f position;
    Vector3f direction;
    float outer_cut_off;
    float far_plane;
};

auto collect_selected_point_lights(const Luminol::Graphics::Light& light_data)
    -> std::vector<SelectedPointLight> {
    auto selected = std::vector<SelectedPointLight>{};

    for (auto i = uint32_t{0}; i < light_data.point_light_count; ++i) {
        const auto& light = gsl::at(light_data.point_lights, i);
        const auto shadow_slot = light.shadow_data.x();
        if (shadow_slot < 0.0F) {
            continue;
        }

        selected.push_back(SelectedPointLight{
            .slot = static_cast<uint32_t>(std::lround(shadow_slot)),
            .position =
                Vector3f{light.position.x(), light.position.y(), light.position.z()},
            .far_plane = light_cull_radius(
                Vector3f{light.color.x(), light.color.y(), light.color.z()}
            ),
        });
    }

    return selected;
}

auto collect_selected_spot_lights(const Luminol::Graphics::Light& light_data)
    -> std::vector<SelectedSpotLight> {
    auto selected = std::vector<SelectedSpotLight>{};

    for (auto i = uint32_t{0}; i < light_data.spot_light_count; ++i) {
        const auto& light = gsl::at(light_data.spot_lights, i);
        if (light.shadow_slot < 0.0F) {
            continue;
        }

        selected.push_back(SelectedSpotLight{
            .slot = static_cast<uint32_t>(std::lround(light.shadow_slot)),
            .position =
                Vector3f{light.position.x(), light.position.y(), light.position.z()},
            .direction =
                Vector3f{light.direction.x(), light.direction.y(), light.direction.z()},
            .outer_cut_off = light.outer_cut_off,
            .far_plane = light_cull_radius(light.color),
        });
    }

    return selected;
}

auto build_and_upload_spot_shadow_matrices(
    CommandBuffer& command_buffer,
    gsl::span<const SelectedSpotLight> selected_spot_lights,
    TransferBuffer& spot_shadow_matrix_transfer_buffer,
    Buffer& spot_shadow_matrix_buffer
) -> std::vector<Matrix4x4f> {
    auto spot_shadow_matrices = std::vector<Matrix4x4f>(
        max_shadow_casting_spot_lights, Matrix4x4f::identity()
    );
    for (const auto& spot_light : selected_spot_lights) {
        if (spot_light.slot >= max_shadow_casting_spot_lights) {
            continue;
        }
        spot_shadow_matrices[spot_light.slot] = spot_light_view_projection(
            spot_light.position, spot_light.direction, spot_light.outer_cut_off,
            spot_light.far_plane
        );
    }

    {
        auto copy_pass = command_buffer.begin_copy_pass();
        const auto size = spot_shadow_matrix_buffer_size;
        const auto mapped = spot_shadow_matrix_transfer_buffer.map(true);
        std::memcpy(mapped.data(), spot_shadow_matrices.data(), size);
        spot_shadow_matrix_transfer_buffer.unmap();
        copy_pass.upload_to_buffer(
            spot_shadow_matrix_transfer_buffer, 0, spot_shadow_matrix_buffer, 0,
            size, true
        );
    }

    return spot_shadow_matrices;
}

struct IndirectCommandResult {
    std::vector<IndirectDrawCommand> indirect_commands;
    std::vector<IndirectDrawRange> point_ranges;
    std::vector<IndirectDrawRange> spot_ranges;
};

// Phase 1: build every (tile, batch)'s indirect draw commands up front, from
// CPU AABB culling, so each tile's surviving meshes across every batch can be
// uploaded once and submitted as one indirect multi-draw call per batch
// instead of one draw call per mesh. point_ranges/spot_ranges are appended in
// exactly this (point-then-spot) order - the caller's Phase 2 submission
// walks them with a running index in the same order.
auto build_indirect_commands(
    const SDL_GPUFactory& graphics_factory,
    gsl::span<const InstanceBatch> instance_batches,
    gsl::span<const SelectedPointLight> selected_point_lights,
    gsl::span<const SelectedSpotLight> selected_spot_lights,
    const BatchMeshBounds& batch_mesh_world_bounds,
    gsl::span<const Matrix4x4f> spot_shadow_matrices
) -> IndirectCommandResult {
    auto result = IndirectCommandResult{};
    result.point_ranges.reserve(
        selected_point_lights.size() * cube_faces_per_light *
        instance_batches.size()
    );
    result.spot_ranges.reserve(selected_spot_lights.size() * instance_batches.size());

    for (const auto& point_light : selected_point_lights) {
        if (point_light.slot >= max_shadow_casting_point_lights) {
            continue;
        }

        for (auto face = uint32_t{0}; face < cube_faces_per_light; ++face) {
            const auto view_projection = point_light_face_view_projection(
                point_light.position, point_light.far_plane, face
            );
            const auto frustum_planes = extract_frustum_planes(view_projection);

            for (auto batch_index = std::size_t{0};
                 batch_index < instance_batches.size(); ++batch_index) {
                result.point_ranges.push_back(append_batch_indirect_commands(
                    graphics_factory, instance_batches[batch_index],
                    batch_mesh_world_bounds[batch_index], frustum_planes,
                    std::nullopt, result.indirect_commands
                ));
            }
        }
    }

    for (const auto& spot_light : selected_spot_lights) {
        if (spot_light.slot >= max_shadow_casting_spot_lights) {
            continue;
        }

        const auto frustum_planes = extract_frustum_planes(
            spot_shadow_matrices[spot_light.slot]
        );

        for (auto batch_index = std::size_t{0};
             batch_index < instance_batches.size(); ++batch_index) {
            result.spot_ranges.push_back(append_batch_indirect_commands(
                graphics_factory, instance_batches[batch_index],
                batch_mesh_world_bounds[batch_index], frustum_planes,
                std::nullopt, result.indirect_commands
            ));
        }
    }

    return result;
}

// Phase 2a: point-light cube-face shadow tiles. Each tile does at most one
// bind + one indirect draw call per batch (skipped entirely if that batch's
// range is empty), instead of one draw call per surviving mesh.
auto record_point_shadow_pass(
    CommandBuffer& command_buffer,
    const SDL_GPUFactory& graphics_factory,
    const SDL_GPUInstanceBufferCache& instance_buffer_cache,
    gsl::span<const InstanceBatch> instance_batches,
    gsl::span<const SelectedPointLight> selected_point_lights,
    gsl::span<const IndirectDrawRange> point_ranges,
    const TextureView& point_shadow_texture_view,
    const GraphicsPipeline& shadow_pipeline,
    const Buffer& indirect_draw_buffer
) -> void {
    const auto depth_stencil_target = DepthStencilTargetInfo{
        .texture = &point_shadow_texture_view,
        .clear_depth = 1.0F,
        .load_op = LoadOp::Clear,
        .store_op = StoreOp::Store,
        .cycle = true,
    };

    auto render_pass =
        command_buffer.begin_render_pass({}, &depth_stencil_target);
    render_pass.bind_graphics_pipeline(shadow_pipeline);

    auto point_range_index = std::size_t{0};
    for (const auto& point_light : selected_point_lights) {
        if (point_light.slot >= max_shadow_casting_point_lights) {
            continue;
        }

        for (auto face = uint32_t{0}; face < cube_faces_per_light; ++face) {
            const auto view_projection = point_light_face_view_projection(
                point_light.position, point_light.far_plane, face
            );

            const auto tile = point_atlas_tile_rect(point_light.slot, face);

            auto tile_has_draws = false;
            for (auto batch_index = std::size_t{0};
                 batch_index < instance_batches.size(); ++batch_index) {
                if (point_ranges[point_range_index + batch_index].count > 0) {
                    tile_has_draws = true;
                    break;
                }
            }

            if (tile_has_draws) {
                render_pass.set_viewport(
                    static_cast<float>(tile.x), static_cast<float>(tile.y),
                    static_cast<float>(tile.size), static_cast<float>(tile.size)
                );
                render_pass.set_scissor(
                    static_cast<int32_t>(tile.x), static_cast<int32_t>(tile.y),
                    static_cast<int32_t>(tile.size),
                    static_cast<int32_t>(tile.size)
                );
                const auto vertex_ubo = VertexUBO{
                    .view_projection = view_projection,
                    .instance_base_offset = {0, 0, 0, 0},
                };
                command_buffer.push_vertex_uniform_data(
                    0,
                    gsl::span{
                        reinterpret_cast<const std::byte*>(&vertex_ubo),
                        sizeof(vertex_ubo)
                    }
                );

                for (auto batch_index = std::size_t{0};
                     batch_index < instance_batches.size(); ++batch_index) {
                    const auto& range =
                        point_ranges[point_range_index + batch_index];
                    if (range.count == 0) {
                        continue;
                    }

                    const auto& batch = instance_batches[batch_index];
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

                    const auto& instance_buffer =
                        instance_buffer_cache.get(batch.renderable_id);
                    const auto& identity_indices_buffer =
                        instance_buffer_cache.get_identity_indices_buffer();
                    const auto storage_buffer_bindings = std::array{
                        &instance_buffer, &identity_indices_buffer
                    };
                    render_pass.bind_vertex_storage_buffers(
                        0, storage_buffer_bindings
                    );

                    render_pass.draw_indexed_primitives_indirect(
                        indirect_draw_buffer,
                        range.offset *
                            static_cast<uint32_t>(sizeof(IndirectDrawCommand)),
                        range.count
                    );
                }
            }

            point_range_index += instance_batches.size();
        }
    }
}

// Phase 2b: spot-light shadow tiles, mirroring record_point_shadow_pass
// without the per-face loop.
auto record_spot_shadow_pass(
    CommandBuffer& command_buffer,
    const SDL_GPUFactory& graphics_factory,
    const SDL_GPUInstanceBufferCache& instance_buffer_cache,
    gsl::span<const InstanceBatch> instance_batches,
    gsl::span<const SelectedSpotLight> selected_spot_lights,
    gsl::span<const IndirectDrawRange> spot_ranges,
    gsl::span<const Matrix4x4f> spot_shadow_matrices,
    const TextureView& spot_shadow_texture_view,
    const GraphicsPipeline& shadow_pipeline,
    const Buffer& indirect_draw_buffer
) -> void {
    const auto depth_stencil_target = DepthStencilTargetInfo{
        .texture = &spot_shadow_texture_view,
        .clear_depth = 1.0F,
        .load_op = LoadOp::Clear,
        .store_op = StoreOp::Store,
        .cycle = true,
    };

    auto render_pass =
        command_buffer.begin_render_pass({}, &depth_stencil_target);
    render_pass.bind_graphics_pipeline(shadow_pipeline);

    auto spot_range_index = std::size_t{0};
    for (const auto& spot_light : selected_spot_lights) {
        if (spot_light.slot >= max_shadow_casting_spot_lights) {
            continue;
        }

        const auto tile = spot_atlas_tile_rect(spot_light.slot);

        auto tile_has_draws = false;
        for (auto batch_index = std::size_t{0};
             batch_index < instance_batches.size(); ++batch_index) {
            if (spot_ranges[spot_range_index + batch_index].count > 0) {
                tile_has_draws = true;
                break;
            }
        }

        if (tile_has_draws) {
            render_pass.set_viewport(
                static_cast<float>(tile.x), static_cast<float>(tile.y),
                static_cast<float>(tile.size), static_cast<float>(tile.size)
            );
            render_pass.set_scissor(
                static_cast<int32_t>(tile.x), static_cast<int32_t>(tile.y),
                static_cast<int32_t>(tile.size), static_cast<int32_t>(tile.size)
            );
            const auto vertex_ubo = VertexUBO{
                .view_projection = spot_shadow_matrices[spot_light.slot],
                .instance_base_offset = {0, 0, 0, 0},
            };
            command_buffer.push_vertex_uniform_data(
                0,
                gsl::span{
                    reinterpret_cast<const std::byte*>(&vertex_ubo),
                    sizeof(vertex_ubo)
                }
            );

            for (auto batch_index = std::size_t{0};
                 batch_index < instance_batches.size(); ++batch_index) {
                const auto& range = spot_ranges[spot_range_index + batch_index];
                if (range.count == 0) {
                    continue;
                }

                const auto& batch = instance_batches[batch_index];
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

                const auto& instance_buffer =
                    instance_buffer_cache.get(batch.renderable_id);
                const auto& identity_indices_buffer =
                    instance_buffer_cache.get_identity_indices_buffer();
                const auto storage_buffer_bindings = std::array{
                    &instance_buffer, &identity_indices_buffer
                };
                render_pass.bind_vertex_storage_buffers(
                    0, storage_buffer_bindings
                );

                render_pass.draw_indexed_primitives_indirect(
                    indirect_draw_buffer,
                    range.offset *
                        static_cast<uint32_t>(sizeof(IndirectDrawCommand)),
                    range.count
                );
            }
        }

        spot_range_index += instance_batches.size();
    }
}

}  // namespace

namespace Luminol::Graphics::SDL_GPU {

SDL_GPUPointSpotShadowPass::SDL_GPUPointSpotShadowPass(GPUDevice& device)
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
      point_shadow_texture{make_point_shadow_texture(device)},
      point_shadow_sampler{make_shadow_map_sampler(device)},
      spot_shadow_texture{make_spot_shadow_texture(device)},
      spot_shadow_sampler{make_shadow_map_sampler(device)},
      spot_shadow_matrix_buffer{device.create_buffer(BufferInfo{
          .usage = BufferUsage::StorageRead,
          .size = spot_shadow_matrix_buffer_size,
      })},
      spot_shadow_matrix_transfer_buffer{device.create_transfer_buffer(TransferBufferInfo{
          .usage = TransferBufferUsage::Upload,
          .size = spot_shadow_matrix_buffer_size,
      })},
      indirect_draw_buffer{device.create_buffer(BufferInfo{
          .usage = BufferUsage::Indirect,
          .size = indirect_draw_buffer_size,
      })},
      indirect_draw_transfer_buffer{device.create_transfer_buffer(TransferBufferInfo{
          .usage = TransferBufferUsage::Upload,
          .size = indirect_draw_buffer_size,
      })} {}

auto SDL_GPUPointSpotShadowPass::draw(
    const SDL_GPUFactory& graphics_factory,
    CommandBuffer& command_buffer,
    const SDL_GPUInstanceBufferCache& instance_buffer_cache,
    gsl::span<const InstanceBatch> instance_batches,
    const std::unordered_map<RenderableId, std::vector<Maths::Matrix4x4f>>&
        queued_draws,
    const Light& light_data,
    Utilities::PerformanceLogger& performance_logger
) -> void {
    const auto pass_timer = Utilities::Timer{};

    const auto selected_point_lights = collect_selected_point_lights(light_data);
    const auto selected_spot_lights = collect_selected_spot_lights(light_data);

    // Skip the O(instance count) CPU AABB computation entirely when no
    // point/spot light is shadow-casting this frame - it's only consumed by
    // append_batch_indirect_commands below, which is a no-op in that case,
    // so an empty BatchMeshBounds is never indexed.
    const auto batch_mesh_world_bounds =
        (selected_point_lights.empty() && selected_spot_lights.empty())
        ? BatchMeshBounds{}
        : compute_batch_mesh_world_bounds(
              graphics_factory, instance_batches, queued_draws
          );

    const auto spot_shadow_matrices = build_and_upload_spot_shadow_matrices(
        command_buffer, selected_spot_lights, spot_shadow_matrix_transfer_buffer,
        spot_shadow_matrix_buffer
    );

    const auto point_shadow_texture_view =
        TextureView{point_shadow_texture.native_handle()};
    const auto spot_shadow_texture_view =
        TextureView{spot_shadow_texture.native_handle()};

    auto [indirect_commands, point_ranges, spot_ranges] = build_indirect_commands(
        graphics_factory, instance_batches, selected_point_lights,
        selected_spot_lights, batch_mesh_world_bounds, spot_shadow_matrices
    );

    Expects(indirect_commands.size() <= max_indirect_draw_commands);

    if (!indirect_commands.empty()) {
        auto copy_pass = command_buffer.begin_copy_pass();
        const auto size = static_cast<uint32_t>(
            indirect_commands.size() * sizeof(IndirectDrawCommand)
        );
        const auto mapped = indirect_draw_transfer_buffer.map(true);
        std::memcpy(mapped.data(), indirect_commands.data(), size);
        indirect_draw_transfer_buffer.unmap();
        copy_pass.upload_to_buffer(
            indirect_draw_transfer_buffer, 0, indirect_draw_buffer, 0, size,
            true
        );
    }

    record_point_shadow_pass(
        command_buffer, graphics_factory, instance_buffer_cache, instance_batches,
        selected_point_lights, point_ranges, point_shadow_texture_view,
        shadow_pipeline, indirect_draw_buffer
    );

    record_spot_shadow_pass(
        command_buffer, graphics_factory, instance_buffer_cache, instance_batches,
        selected_spot_lights, spot_ranges, spot_shadow_matrices,
        spot_shadow_texture_view, shadow_pipeline, indirect_draw_buffer
    );

    performance_logger.record(
        "point_spot_shadow_pass", Units::Seconds{pass_timer.elapsed_seconds()}
    );
}

auto SDL_GPUPointSpotShadowPass::get_point_shadow_texture() const
    -> const Texture& {
    return point_shadow_texture;
}

auto SDL_GPUPointSpotShadowPass::get_point_shadow_sampler() const
    -> const Sampler& {
    return point_shadow_sampler;
}

auto SDL_GPUPointSpotShadowPass::get_spot_shadow_texture() const
    -> const Texture& {
    return spot_shadow_texture;
}

auto SDL_GPUPointSpotShadowPass::get_spot_shadow_sampler() const
    -> const Sampler& {
    return spot_shadow_sampler;
}

auto SDL_GPUPointSpotShadowPass::get_spot_shadow_matrix_buffer() const
    -> const Buffer& {
    return spot_shadow_matrix_buffer;
}

}  // namespace Luminol::Graphics::SDL_GPU
