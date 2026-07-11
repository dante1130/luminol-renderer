#include "SDL_GPUShadowPass.hpp"

#include <array>
#include <cmath>

#include <LuminolMaths/Transform.hpp>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCommandBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUDevice.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUFactory.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPURenderPass.hpp>
#include <LuminolRenderEngine/Utilities/Timer.hpp>

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

constexpr auto shadow_map_format = TextureFormat::D24_Unorm;

constexpr auto shadow_map_resolution = 2048U;
constexpr auto shadow_distance = 30.0F;
constexpr auto shadow_ortho_half_extent = 18.0F;
constexpr auto shadow_near_plane = 0.1F;
constexpr auto shadow_far_plane = shadow_distance + shadow_ortho_half_extent;

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

auto compute_light_space_matrix(
    const Vector3f& light_direction, const Vector3f& camera_position
) -> Matrix4x4f {
    const auto direction = light_direction.normalized();
    const auto light_up_vector =
        std::abs(direction.dot(world_up)) > 0.99F ? up_fallback : world_up;

    // Stabilize the shadow map against camera movement: snap the
    // camera-following target to whole shadow-map texels in the light's
    // right/up axes so the texel grid only ever shifts by whole texels
    // frame-to-frame, preventing sub-texel shimmer at shadow edges.
    const auto right = light_up_vector.cross(direction).normalized();
    const auto up_axis = direction.cross(right);

    constexpr auto texel_size =
        (2.0F * shadow_ortho_half_extent) /
        static_cast<float>(shadow_map_resolution);

    const auto snap = [](float value) {
        return std::floor(value / texel_size) * texel_size;
    };

    const auto camera_right_dist = right.dot(camera_position);
    const auto camera_up_dist = up_axis.dot(camera_position);

    const auto snapped_target = camera_position +
        right * (snap(camera_right_dist) - camera_right_dist) +
        up_axis * (snap(camera_up_dist) - camera_up_dist);

    const auto light_eye = snapped_target - direction * shadow_distance;

    const auto light_view = Transform::left_handed_look_at_matrix(
        Transform::LookAtParams<float>{
            .eye = light_eye,
            .target = snapped_target,
            .up_vector = light_up_vector
        }
    );

    const auto light_projection = left_handed_orthographic_projection_matrix(
        shadow_ortho_half_extent,
        shadow_ortho_half_extent,
        shadow_near_plane,
        shadow_far_plane
    );

    return light_view * light_projection;
}

auto make_shadow_map_texture(GPUDevice& device) -> Texture {
    return device.create_texture(TextureInfo{
        .width = shadow_map_resolution,
        .height = shadow_map_resolution,
        .format = shadow_map_format,
        .usage = TextureUsage::DepthStencilTarget | TextureUsage::Sampler,
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

}  // namespace

namespace Luminol::Graphics::SDL_GPU {

SDL_GPUShadowPass::SDL_GPUShadowPass(GPUDevice& device)
    : shadow_vertex_shader{make_shader(
          device,
          "res/shaders/sdl_gpu/pbr_vert.hlsl",
          ShaderStage::Vertex,
          1U,
          2U
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
      shadow_map_texture{make_shadow_map_texture(device)},
      shadow_map_sampler{device.create_sampler(SamplerInfo{
          .filter = SamplerFilter::Linear,
          .address_mode_u = SamplerAddressMode::ClampToEdge,
          .address_mode_v = SamplerAddressMode::ClampToEdge,
          .enable_compare = true,
      })} {}

auto SDL_GPUShadowPass::draw(
    const SDL_GPUFactory& graphics_factory,
    CommandBuffer& command_buffer,
    const SDL_GPUInstanceBufferCache& instance_buffer_cache,
    gsl::span<const InstanceBatch> instance_batches,
    const Maths::Vector3f& light_direction,
    const Maths::Vector3f& camera_position,
    Utilities::PerformanceLogger& performance_logger
) -> void {
    const auto pass_timer = Utilities::Timer{};

    light_space_matrix =
        compute_light_space_matrix(light_direction, camera_position);

    const auto shadow_map_texture_view =
        TextureView{shadow_map_texture.native_handle()};

    const auto depth_stencil_target = DepthStencilTargetInfo{
        .texture = &shadow_map_texture_view,
        .clear_depth = 1.0F,
        .load_op = LoadOp::Clear,
        .store_op = StoreOp::Store,
        .cycle = true,
    };

    auto render_pass =
        command_buffer.begin_render_pass({}, &depth_stencil_target);
    render_pass.bind_graphics_pipeline(shadow_pipeline);

    const auto vertex_ubo = VertexUBO{
        .light_space_matrix = light_space_matrix,
        .instance_base_offset = {0, 0, 0, 0},
    };
    command_buffer.push_vertex_uniform_data(
        0,
        gsl::span{
            reinterpret_cast<const std::byte*>(&vertex_ubo), sizeof(vertex_ubo)
        }
    );

    for (const auto& batch : instance_batches) {
        const auto& instance_buffer =
            instance_buffer_cache.get(batch.renderable_id);
        const auto& identity_indices_buffer =
            instance_buffer_cache.get_identity_indices_buffer();
        const auto storage_buffer_bindings =
            std::array{&instance_buffer, &identity_indices_buffer};
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

        for (const auto& mesh :
             graphics_factory.get_meshes(batch.renderable_id)) {
            mesh.draw_instanced_geometry_only(
                static_cast<int32_t>(batch.instance_count), render_pass
            );
        }
    }

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

auto SDL_GPUShadowPass::get_light_space_matrix() const
    -> const Maths::Matrix4x4f& {
    return light_space_matrix;
}

}  // namespace Luminol::Graphics::SDL_GPU
