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
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUDevice.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUFactory.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPURenderPass.hpp>
#include <LuminolRenderEngine/Utilities/Timer.hpp>

namespace {

using namespace Luminol::Graphics;
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

auto make_shadow_pipeline(
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
        .depth_stencil_format = shadow_map_format,
        .cull_mode = CullMode::Back,
        .front_face = FrontFace::Clockwise,
    });
}

// Transforms a local-space point by a row-major matrix under row-vector
// convention (p * M, see pbr_vert.hlsl's mul(pos, vp)).
auto transform_point(const Matrix4x4f& matrix, const Vector3f& point)
    -> Vector3f {
    return Vector3f{
        (point.x() * matrix[0][0]) + (point.y() * matrix[1][0]) +
            (point.z() * matrix[2][0]) + matrix[3][0],
        (point.x() * matrix[0][1]) + (point.y() * matrix[1][1]) +
            (point.z() * matrix[2][1]) + matrix[3][1],
        (point.x() * matrix[0][2]) + (point.y() * matrix[1][2]) +
            (point.z() * matrix[2][2]) + matrix[3][2],
    };
}

// One world-space AABB per mesh (submesh) within a batch, covering the union
// of all of that batch's current-frame instances. Computed once per frame
// and reused across every light-face pass, since it doesn't depend on the
// light.
using BatchMeshBounds = std::vector<std::vector<Luminol::Graphics::BoundingBox>>;

auto compute_batch_mesh_world_bounds(
    const SDL_GPUFactory& graphics_factory,
    gsl::span<const InstanceBatch> instance_batches,
    const std::unordered_map<RenderableId, std::vector<Matrix4x4f>>&
        queued_draws
) -> BatchMeshBounds {
    auto result = BatchMeshBounds{};
    result.reserve(instance_batches.size());

    for (const auto& batch : instance_batches) {
        const auto meshes = graphics_factory.get_meshes(batch.renderable_id);
        auto mesh_bounds = std::vector<Luminol::Graphics::BoundingBox>{};
        mesh_bounds.reserve(meshes.size());

        const auto transforms_it = queued_draws.find(batch.renderable_id);
        const auto has_transforms = transforms_it != queued_draws.end() &&
            !transforms_it->second.empty();

        for (const auto& mesh : meshes) {
            const auto& local_bounds = mesh.get_local_bounds();

            if (!has_transforms) {
                mesh_bounds.push_back(local_bounds);
                continue;
            }

            const auto local_corners = std::array{
                Vector3f{local_bounds.min.x(), local_bounds.min.y(), local_bounds.min.z()},
                Vector3f{local_bounds.max.x(), local_bounds.min.y(), local_bounds.min.z()},
                Vector3f{local_bounds.min.x(), local_bounds.max.y(), local_bounds.min.z()},
                Vector3f{local_bounds.max.x(), local_bounds.max.y(), local_bounds.min.z()},
                Vector3f{local_bounds.min.x(), local_bounds.min.y(), local_bounds.max.z()},
                Vector3f{local_bounds.max.x(), local_bounds.min.y(), local_bounds.max.z()},
                Vector3f{local_bounds.min.x(), local_bounds.max.y(), local_bounds.max.z()},
                Vector3f{local_bounds.max.x(), local_bounds.max.y(), local_bounds.max.z()},
            };

            auto world_min = Vector3f{0.0F, 0.0F, 0.0F};
            auto world_max = Vector3f{0.0F, 0.0F, 0.0F};
            auto bounds_initialized = false;

            for (const auto& transform : transforms_it->second) {
                for (const auto& corner : local_corners) {
                    const auto world_corner = transform_point(transform, corner);

                    if (!bounds_initialized) {
                        world_min = world_corner;
                        world_max = world_corner;
                        bounds_initialized = true;
                        continue;
                    }

                    world_min = Vector3f{
                        std::min(world_min.x(), world_corner.x()),
                        std::min(world_min.y(), world_corner.y()),
                        std::min(world_min.z(), world_corner.z()),
                    };
                    world_max = Vector3f{
                        std::max(world_max.x(), world_corner.x()),
                        std::max(world_max.y(), world_corner.y()),
                        std::max(world_max.z(), world_corner.z()),
                    };
                }
            }

            mesh_bounds.push_back(Luminol::Graphics::BoundingBox{
                .min = world_min,
                .max = world_max,
            });
        }

        result.push_back(std::move(mesh_bounds));
    }

    return result;
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

}  // namespace

namespace Luminol::Graphics::SDL_GPU {

SDL_GPUPointSpotShadowPass::SDL_GPUPointSpotShadowPass(GPUDevice& device)
    : shadow_vertex_shader{make_shader(
          device,
          "res/shaders/sdl_gpu/pbr_vert.hlsl",
          ShaderStage::Vertex,
          1U,
          1U
      )},
      shadow_fragment_shader{make_shader(
          device,
          "res/shaders/sdl_gpu/shadow_depth_frag.hlsl",
          ShaderStage::Fragment,
          0U,
          0U
      )},
      shadow_pipeline{make_shadow_pipeline(
          device, shadow_vertex_shader, shadow_fragment_shader
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

    const auto batch_mesh_world_bounds = compute_batch_mesh_world_bounds(
        graphics_factory, instance_batches, queued_draws
    );

    const auto selected_point_lights = collect_selected_point_lights(light_data);
    const auto selected_spot_lights = collect_selected_spot_lights(light_data);

    auto spot_shadow_matrices = std::vector<Maths::Matrix4x4f>(
        max_shadow_casting_spot_lights, Maths::Matrix4x4f::identity()
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

    const auto point_shadow_texture_view =
        TextureView{point_shadow_texture.native_handle()};
    const auto spot_shadow_texture_view =
        TextureView{spot_shadow_texture.native_handle()};

    // Culls and draws into whichever tile the caller has already pointed the
    // viewport/scissor at, pushing the view-projection uniform lazily (only
    // once something actually survives culling for this tile). The pipeline
    // itself is bound once per atlas by the caller, not per tile.
    const auto draw_instance_batches_in_tile =
        [&](RenderPass& render_pass, const Matrix4x4f& view_projection) {
            const auto frustum_planes = extract_frustum_planes(view_projection);

            auto uniform_pushed = false;

            for (auto batch_index = std::size_t{0};
                 batch_index < instance_batches.size(); ++batch_index) {
                const auto& batch = instance_batches[batch_index];
                const auto meshes =
                    graphics_factory.get_meshes(batch.renderable_id);
                const auto& mesh_bounds = batch_mesh_world_bounds[batch_index];

                auto bound_storage_buffer = false;

                for (auto mesh_index = std::size_t{0};
                     mesh_index < meshes.size(); ++mesh_index) {
                    const auto& bounds = mesh_bounds[mesh_index];
                    if (!aabb_in_frustum(
                            frustum_planes, bounds.min, bounds.max
                        )) {
                        continue;
                    }

                    if (!uniform_pushed) {
                        command_buffer.push_vertex_uniform_data(
                            0,
                            gsl::span{
                                reinterpret_cast<const std::byte*>(
                                    &view_projection
                                ),
                                sizeof(view_projection)
                            }
                        );
                        uniform_pushed = true;
                    }

                    if (!bound_storage_buffer) {
                        const auto& instance_buffer =
                            instance_buffer_cache.get(batch.renderable_id);
                        const auto storage_buffer_bindings =
                            std::array{&instance_buffer};
                        render_pass.bind_vertex_storage_buffers(
                            0, storage_buffer_bindings
                        );
                        bound_storage_buffer = true;
                    }

                    meshes[mesh_index].draw_instanced_geometry_only(
                        static_cast<int32_t>(batch.instance_count), render_pass
                    );
                }
            }
        };

    {
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

        for (const auto& point_light : selected_point_lights) {
            if (point_light.slot >= max_shadow_casting_point_lights) {
                continue;
            }

            for (auto face = uint32_t{0}; face < cube_faces_per_light; ++face) {
                const auto view_projection = point_light_face_view_projection(
                    point_light.position, point_light.far_plane, face
                );

                const auto tile = point_atlas_tile_rect(point_light.slot, face);
                render_pass.set_viewport(
                    static_cast<float>(tile.x), static_cast<float>(tile.y),
                    static_cast<float>(tile.size), static_cast<float>(tile.size)
                );
                render_pass.set_scissor(
                    static_cast<int32_t>(tile.x), static_cast<int32_t>(tile.y),
                    static_cast<int32_t>(tile.size), static_cast<int32_t>(tile.size)
                );

                draw_instance_batches_in_tile(render_pass, view_projection);
            }
        }
    }

    {
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

        for (const auto& spot_light : selected_spot_lights) {
            if (spot_light.slot >= max_shadow_casting_spot_lights) {
                continue;
            }

            const auto tile = spot_atlas_tile_rect(spot_light.slot);
            render_pass.set_viewport(
                static_cast<float>(tile.x), static_cast<float>(tile.y),
                static_cast<float>(tile.size), static_cast<float>(tile.size)
            );
            render_pass.set_scissor(
                static_cast<int32_t>(tile.x), static_cast<int32_t>(tile.y),
                static_cast<int32_t>(tile.size), static_cast<int32_t>(tile.size)
            );

            draw_instance_batches_in_tile(
                render_pass, spot_shadow_matrices[spot_light.slot]
            );
        }
    }

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
