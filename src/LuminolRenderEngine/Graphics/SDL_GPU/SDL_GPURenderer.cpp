#include "SDL_GPURenderer.hpp"

#include <array>
#include <cmath>
#include <numbers>
#include <utility>

#include <gsl/gsl>
#include <SDL3/SDL_video.h>

#include <cstring>

#include <SDL3/SDL_log.h>

#include <LuminolRenderEngine/Graphics/Frustum.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCommandBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCopyPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCullingUtils.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUFactory.hpp>
#include <LuminolRenderEngine/Utilities/Timer.hpp>

namespace {

using namespace Luminol::Graphics::SDL_GPU;
using namespace Luminol::Maths;

constexpr auto depth_texture_format = TextureFormat::D24_Unorm;
constexpr auto hdr_color_texture_format = TextureFormat::R16G16B16A16_Float;
constexpr auto shadow_normal_offset_bias = 0.05F;

struct CameraParams {
    float vertical_fov_degrees;
    float aspect_ratio;
    float near_plane;
    float far_plane;
};

// Recovers the camera parameters used to build `projection_matrix` (see
// Maths::Transform::left_handed_perspective_projection_matrix) directly from
// the matrix, so SDL_GPURenderer doesn't need a separate camera-params API -
// Renderer only ever receives already-computed matrices.
auto extract_camera_params(const Matrix4x4f& projection_matrix) -> CameraParams {
    const auto a = projection_matrix[0][0];
    const auto b = projection_matrix[1][1];
    const auto c = projection_matrix[2][2];
    const auto e = projection_matrix[3][2];

    const auto tan_half_fov_y = 1.0F / b;
    const auto vertical_fov_degrees = 2.0F * std::atan(tan_half_fov_y) *
        (180.0F / std::numbers::pi_v<float>);
    const auto aspect_ratio = b / a;
    const auto near_plane = -e / c;
    const auto far_plane = e / (1.0F - c);

    return CameraParams{
        .vertical_fov_degrees = vertical_fov_degrees,
        .aspect_ratio = aspect_ratio,
        .near_plane = near_plane,
        .far_plane = far_plane,
    };
}

auto get_view_position(const Matrix4x4f& view_matrix) -> Vector4f {
    const auto inverse_view_matrix = view_matrix.inverse();

    return Vector4f{
        inverse_view_matrix[3][0],
        inverse_view_matrix[3][1],
        inverse_view_matrix[3][2],
        0.0F,
    };
}

auto make_depth_texture(GPUDevice& device, uint32_t width, uint32_t height)
    -> Texture {
    return device.create_texture(TextureInfo{
        .width = width,
        .height = height,
        .format = depth_texture_format,
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

auto make_hdr_color_texture(GPUDevice& device, uint32_t width, uint32_t height)
    -> Texture {
    return device.create_texture(TextureInfo{
        .width = width,
        .height = height,
        .format = hdr_color_texture_format,
        .usage = TextureUsage::ColorTarget | TextureUsage::Sampler,
    });
}

auto make_hdr_color_texture(GPUDevice& device, SDL_Window* window) -> Texture {
    auto width = int{0};
    auto height = int{0};
    SDL_GetWindowSizeInPixels(window, &width, &height);

    return make_hdr_color_texture(
        device, static_cast<uint32_t>(width), static_cast<uint32_t>(height)
    );
}

auto make_msaa_color_texture(
    GPUDevice& device, uint32_t width, uint32_t height, SampleCount sample_count
) -> Texture {
    return device.create_texture(TextureInfo{
        .width = width,
        .height = height,
        .format = hdr_color_texture_format,
        .usage = TextureUsage::ColorTarget,
        .sample_count = sample_count,
    });
}

auto make_msaa_color_texture(
    GPUDevice& device, SDL_Window* window, SampleCount sample_count
) -> Texture {
    auto width = int{0};
    auto height = int{0};
    SDL_GetWindowSizeInPixels(window, &width, &height);

    return make_msaa_color_texture(
        device, static_cast<uint32_t>(width), static_cast<uint32_t>(height),
        sample_count
    );
}

auto make_hiz_debug_shader(
    GPUDevice& device, const std::filesystem::path& path, ShaderStage stage,
    uint32_t sampler_count, uint32_t uniform_buffer_count
) -> Shader {
    return device.create_shader(ShaderInfo{
        .path = path,
        .stage = stage,
        .source_language = ShaderSourceLanguage::Hlsl,
        .sampler_count = sampler_count,
        .uniform_buffer_count = uniform_buffer_count,
    });
}

// Mirrors cbuffer DebugVisualizeParams in hiz_debug_visualize_frag.hlsl.
struct HiZDebugVisualizeParams {
    float near_plane;
    float far_plane;
    std::array<float, 2> padding;
};

auto make_hiz_debug_pipeline(
    GPUDevice& device, SDL_Window* window, const Shader& vertex_shader,
    const Shader& fragment_shader
) -> GraphicsPipeline {
    return device.create_graphics_pipeline(GraphicsPipelineInfo{
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_target_format = device.get_swapchain_texture_format(window),
        .primitive_type = PrimitiveType::TriangleList,
        .vertex_buffer_descriptions = {},
        .vertex_attributes = {},
        .enable_depth_test = false,
        .cull_mode = CullMode::None,
    });
}

auto make_msaa_depth_texture(
    GPUDevice& device, uint32_t width, uint32_t height, SampleCount sample_count
) -> Texture {
    return device.create_texture(TextureInfo{
        .width = width,
        .height = height,
        .format = depth_texture_format,
        .usage = TextureUsage::DepthStencilTarget,
        .sample_count = sample_count,
    });
}

auto make_msaa_depth_texture(
    GPUDevice& device, SDL_Window* window, SampleCount sample_count
) -> Texture {
    auto width = int{0};
    auto height = int{0};
    SDL_GetWindowSizeInPixels(window, &width, &height);

    return make_msaa_depth_texture(
        device, static_cast<uint32_t>(width), static_cast<uint32_t>(height),
        sample_count
    );
}

// Steps down from the requested sample count until one is supported by the
// device for both the HDR color format and the depth format used by the main
// pass's MSAA targets, falling back to x1 (MSAA disabled) if none match.
auto clamp_supported_sample_count(const GPUDevice& device, SampleCount requested)
    -> SampleCount {
    const auto supported = [&device](SampleCount candidate) {
        return device.supports_sample_count(hdr_color_texture_format, candidate) &&
            device.supports_sample_count(depth_texture_format, candidate);
    };

    switch (requested) {
        case SampleCount::x8:
            if (supported(SampleCount::x8)) {
                return SampleCount::x8;
            }
            [[fallthrough]];
        case SampleCount::x4:
            if (supported(SampleCount::x4)) {
                return SampleCount::x4;
            }
            [[fallthrough]];
        case SampleCount::x2:
            if (supported(SampleCount::x2)) {
                return SampleCount::x2;
            }
            [[fallthrough]];
        case SampleCount::x1:
            return SampleCount::x1;
    }
    return SampleCount::x1;
}

}  // namespace

namespace Luminol::Graphics::SDL_GPU {

SDL_GPURenderer::SDL_GPURenderer(
    Window& window,
    std::shared_ptr<SDL_GPUFactory> graphics_factory,
    std::shared_ptr<GPUDevice> gpu_device,
    SampleCount requested_msaa_sample_count
)
    : Renderer(graphics_factory),
      sdl_window{static_cast<SDL_Window*>(window.get_window_handle())},
      sdl_gpu_factory{std::move(graphics_factory)},
      gpu_device{std::move(gpu_device)},
      mesh_render_pass{
          *this->gpu_device,
          clamp_supported_sample_count(
              *this->gpu_device, requested_msaa_sample_count
          )
      },
      ao_pass{*this->gpu_device, sdl_window},
      ssr_pass{*this->gpu_device, sdl_window},
      hiz_pass{*this->gpu_device, sdl_window},
      phase1_cull_pass{*this->gpu_device},
      occlusion_depth_pass{*this->gpu_device, sdl_window},
      instance_cull_pass{*this->gpu_device},
      cluster_pass{*this->gpu_device},
      shadow_pass{*this->gpu_device},
      point_spot_shadow_pass{*this->gpu_device},
      tonemap_pass{*this->gpu_device, sdl_window},
      skybox_render_pass{
          *this->gpu_device,
          clamp_supported_sample_count(
              *this->gpu_device, requested_msaa_sample_count
          )
      },
      ibl_render_pass{
          *this->gpu_device,
          skybox_render_pass.get_skybox_texture(),
          skybox_render_pass.get_skybox_sampler()
      },
      text_render_pass{*this->gpu_device, sdl_window},
      depth_texture{make_depth_texture(*this->gpu_device, sdl_window)},
      hdr_color_texture{make_hdr_color_texture(*this->gpu_device, sdl_window)},
      previous_hdr_color_texture{
          make_hdr_color_texture(*this->gpu_device, sdl_window)
      },
      point_sampler{this->gpu_device->create_sampler(SamplerInfo{
          .filter = SamplerFilter::Nearest,
          .address_mode_u = SamplerAddressMode::ClampToEdge,
          .address_mode_v = SamplerAddressMode::ClampToEdge,
      })},
      msaa_sample_count{clamp_supported_sample_count(
          *this->gpu_device, requested_msaa_sample_count
      )},
      msaa_color_texture{make_msaa_color_texture(
          *this->gpu_device, sdl_window, msaa_sample_count
      )},
      msaa_depth_texture{make_msaa_depth_texture(
          *this->gpu_device, sdl_window, msaa_sample_count
      )},
      hiz_debug_vertex_shader{make_hiz_debug_shader(
          *this->gpu_device, "res/shaders/sdl_gpu/fullscreen_vert.hlsl",
          ShaderStage::Vertex, 0U, 0U
      )},
      hiz_debug_fragment_shader{make_hiz_debug_shader(
          *this->gpu_device, "res/shaders/sdl_gpu/hiz_debug_visualize_frag.hlsl",
          ShaderStage::Fragment, 1U, 1U
      )},
      hiz_debug_pipeline{make_hiz_debug_pipeline(
          *this->gpu_device, sdl_window, hiz_debug_vertex_shader,
          hiz_debug_fragment_shader
      )},
      hiz_debug_sampler{this->gpu_device->create_sampler(SamplerInfo{
          .filter = SamplerFilter::Nearest,
          .address_mode_u = SamplerAddressMode::ClampToEdge,
          .address_mode_v = SamplerAddressMode::ClampToEdge,
      })} {}

auto SDL_GPURenderer::set_view_matrix(const Maths::Matrix4x4f& view_matrix)
    -> void {
    this->view_matrix = view_matrix;
}

auto SDL_GPURenderer::set_projection_matrix(
    const Maths::Matrix4x4f& projection_matrix
) -> void {
    this->projection_matrix = projection_matrix;
}

auto SDL_GPURenderer::set_exposure(float exposure) -> void {
    this->exposure = exposure;
}

auto SDL_GPURenderer::clear_color(const Maths::Vector4f& color) const -> void {
    clear_color_value = color;
}

auto SDL_GPURenderer::queue_draw(
    RenderableId renderable_id, const Maths::Matrix4x4f& model_matrix
) -> void {
    queued_draws[renderable_id].push_back(model_matrix);
}

auto SDL_GPURenderer::queue_draw_instanced(
    RenderableId renderable_id, gsl::span<const Maths::Matrix4x4f> model_matrices
) -> void {
    auto& batch = queued_draws[renderable_id];
    batch.insert(batch.end(), model_matrices.begin(), model_matrices.end());
}

auto SDL_GPURenderer::draw() -> void {
    const auto frame_timer = Utilities::Timer{};

    auto command_buffer = gpu_device->create_command_buffer();

    const auto acquire_timer = Utilities::Timer{};
    const auto swapchain = command_buffer.acquire_swapchain_texture(sdl_window);
    performance_logger.record(
        "acquire_swapchain", Units::Seconds{acquire_timer.elapsed_seconds()}
    );

    if (!swapchain.has_value()) {
        command_buffer.submit();
        queued_draws.clear();
        return;
    }

    if (depth_texture.get_width() != swapchain->width ||
        depth_texture.get_height() != swapchain->height) {
        depth_texture = make_depth_texture(
            *gpu_device, swapchain->width, swapchain->height
        );
        hdr_color_texture = make_hdr_color_texture(
            *gpu_device, swapchain->width, swapchain->height
        );
        previous_hdr_color_texture = make_hdr_color_texture(
            *gpu_device, swapchain->width, swapchain->height
        );
        has_valid_previous_hdr = false;
        msaa_color_texture = make_msaa_color_texture(
            *gpu_device, swapchain->width, swapchain->height, msaa_sample_count
        );
        msaa_depth_texture = make_msaa_depth_texture(
            *gpu_device, swapchain->width, swapchain->height, msaa_sample_count
        );
        ao_pass.resize(*gpu_device, swapchain->width, swapchain->height);
        ssr_pass.resize(*gpu_device, swapchain->width, swapchain->height);
        hiz_pass.resize(*gpu_device, swapchain->width, swapchain->height);
        occlusion_depth_pass.resize(
            *gpu_device, swapchain->width, swapchain->height
        );
        has_valid_previous_depth = false;
    }

    const auto hdr_color_texture_view =
        TextureView{hdr_color_texture.native_handle()};
    const auto msaa_color_texture_view =
        TextureView{msaa_color_texture.native_handle()};
    const auto msaa_depth_texture_view =
        TextureView{msaa_depth_texture.native_handle()};

    const auto color_targets = std::array{ColorTargetInfo{
        .texture = &msaa_color_texture_view,
        .clear_color = clear_color_value,
        .load_op = LoadOp::Clear,
        .store_op = StoreOp::Resolve,
        .resolve_texture = &hdr_color_texture_view,
    }};

    auto instance_batches = std::vector<InstanceBatch>{};
    {
        const auto pass_timer = Utilities::Timer{};

        auto copy_pass = command_buffer.begin_copy_pass();
        instance_batches =
            mesh_render_pass.upload_instances(*gpu_device, copy_pass, queued_draws);

        performance_logger.record(
            "instance_upload", Units::Seconds{pass_timer.elapsed_seconds()}
        );
    }

    const auto current_view_projection = view_matrix * projection_matrix;
    const auto camera_frustum_planes =
        extract_frustum_planes(current_view_projection);

    // Phase 1: cheap early-out cull against LAST FRAME's Hi-Z (built from
    // depth_texture, which still holds last frame's contents - nothing has
    // written it yet this frame), using THIS frame's camera. Its result only
    // bootstraps occlusion_depth_pass's same-frame depth below and is never
    // used for final shading, so any staleness here only costs extra draws
    // in phase 2, never incorrect final visibility. Must happen before any
    // render pass is opened this frame - it uploads via a copy pass and
    // dispatches compute passes, and SDL_GPU forbids beginning either while
    // a render pass is active.
    hiz_pass.build(command_buffer, depth_texture, point_sampler);

    const auto phase1_cull_layout = phase1_cull_pass.cull(
        *this->sdl_gpu_factory, command_buffer,
        mesh_render_pass.get_instance_buffer_cache(), instance_batches,
        camera_frustum_planes, current_view_projection,
        hiz_pass.get_pyramid_texture(), hiz_pass.get_pyramid_sampler(),
        (has_valid_previous_depth && !debug_disable_occlusion_culling)
            ? hiz_pass.get_mip_levels()
            : 0U
    );

    occlusion_depth_pass.draw(
        *this->sdl_gpu_factory, command_buffer,
        mesh_render_pass.get_instance_buffer_cache(), instance_batches,
        view_matrix, projection_matrix,
        phase1_cull_pass.get_indirect_command_buffer(),
        phase1_cull_pass.get_visible_instance_indices_buffer(),
        phase1_cull_layout
    );

    // Phase 2: rebuild the Hi-Z pyramid from THIS FRAME's own (phase 1)
    // depth - same camera, no cross-frame mismatch, so this cull is always
    // correct regardless of camera speed or scene density (no
    // has_valid_previous_depth gate needed - occlusion_depth_pass's draw
    // above always produces a complete, accurate, same-frame depth to
    // build from, even if phase 1 itself under-culled on a cold-start
    // frame).
    hiz_pass.build(
        command_buffer, occlusion_depth_pass.get_depth_texture(), point_sampler
    );

    // Debug-only: short-circuit the rest of the frame and blit the Hi-Z
    // pyramid's mip 0 straight to the screen instead of the normal scene, to
    // visually sanity-check its contents. Inspects the phase-2 (same-frame)
    // pyramid, since that's the one the final cull below actually uses.
    if (debug_visualize_hiz) {
        const auto visualize_color_targets = std::array{ColorTargetInfo{
            .texture = &swapchain->texture,
            .load_op = LoadOp::DontCare,
            .store_op = StoreOp::Store,
        }};

        {
            auto visualize_render_pass =
                command_buffer.begin_render_pass(visualize_color_targets);
            visualize_render_pass.bind_graphics_pipeline(hiz_debug_pipeline);

            const auto debug_camera_params =
                extract_camera_params(projection_matrix);
            const auto debug_visualize_params = HiZDebugVisualizeParams{
                .near_plane = debug_camera_params.near_plane,
                .far_plane = debug_camera_params.far_plane,
                .padding = {0.0F, 0.0F},
            };
            command_buffer.push_fragment_uniform_data(
                0,
                gsl::span{
                    reinterpret_cast<const std::byte*>(&debug_visualize_params),
                    sizeof(debug_visualize_params)
                }
            );

            const auto hiz_sampler_bindings = std::array{TextureSamplerBinding{
                .texture = &hiz_pass.get_pyramid_texture(),
                .sampler = &hiz_debug_sampler,
            }};
            visualize_render_pass.bind_fragment_samplers(
                0, hiz_sampler_bindings
            );

            visualize_render_pass.draw_primitives(3, 1, 0, 0);
        }

        command_buffer.submit();
        queued_draws.clear();
        return;
    }

    // Phase 2 cull: the final, always-correct visible set, consumed by every
    // downstream pass below exactly as the single-phase design did before.
    const auto instance_cull_layout = instance_cull_pass.cull(
        *this->sdl_gpu_factory, command_buffer,
        mesh_render_pass.get_instance_buffer_cache(), instance_batches,
        camera_frustum_planes, current_view_projection,
        hiz_pass.get_pyramid_texture(), hiz_pass.get_pyramid_sampler(),
        debug_disable_occlusion_culling ? 0U : hiz_pass.get_mip_levels()
    );

    ao_pass.draw(
        *this->sdl_gpu_factory,
        command_buffer,
        mesh_render_pass.get_instance_buffer_cache(),
        instance_batches,
        view_matrix,
        projection_matrix,
        instance_cull_pass.get_indirect_command_buffer(),
        instance_cull_pass.get_visible_instance_indices_buffer(),
        instance_cull_layout,
        depth_texture,
        performance_logger
    );

    // Screen-space reflections: trace against this frame's depth + normals
    // (just written by the AO prepass) and sample the previous frame's
    // resolved HDR color. Consumed by the main pass below.
    ssr_pass.draw(
        command_buffer,
        projection_matrix,
        depth_texture,
        ao_pass.get_normal_texture(),
        previous_hdr_color_texture,
        has_valid_previous_hdr,
        performance_logger
    );

    const auto camera_position = get_view_position(view_matrix);
    const auto camera_params = extract_camera_params(projection_matrix);

    get_light_manager().update_shadow_casters(
        view_matrix * projection_matrix,
        Maths::Vector3f{
            camera_position.x(), camera_position.y(), camera_position.z()
        }
    );

    const auto& light_manager_data = get_light_manager().get_light_data();
    const auto& directional_light = light_manager_data.directional_light;

    {
        const auto pass_timer = Utilities::Timer{};
        cluster_pass.build_cluster_grid(
            command_buffer,
            camera_params.vertical_fov_degrees,
            camera_params.aspect_ratio,
            camera_params.near_plane,
            camera_params.far_plane
        );
        performance_logger.record(
            "cluster_build", Units::Seconds{pass_timer.elapsed_seconds()}
        );
    }

    {
        const auto pass_timer = Utilities::Timer{};
        cluster_pass.cull_lights(command_buffer, light_manager_data, view_matrix);
        performance_logger.record(
            "cluster_cull", Units::Seconds{pass_timer.elapsed_seconds()}
        );
    }

    shadow_pass.draw(
        *this->sdl_gpu_factory,
        command_buffer,
        mesh_render_pass.get_instance_buffer_cache(),
        instance_batches,
        Maths::Vector3f{
            directional_light.direction.x(),
            directional_light.direction.y(),
            directional_light.direction.z()
        },
        Maths::Vector3f{
            camera_position.x(), camera_position.y(), camera_position.z()
        },
        performance_logger
    );

    point_spot_shadow_pass.draw(
        *this->sdl_gpu_factory,
        command_buffer,
        mesh_render_pass.get_instance_buffer_cache(),
        instance_batches,
        queued_draws,
        light_manager_data,
        performance_logger
    );

    const auto depth_stencil_target = DepthStencilTargetInfo{
        .texture = &msaa_depth_texture_view,
        .clear_depth = 1.0F,
        .load_op = LoadOp::Clear,
        .store_op = StoreOp::DontCare,
    };

    {
        const auto pass_timer = Utilities::Timer{};

        auto render_pass = command_buffer.begin_render_pass(
            color_targets, &depth_stencil_target
        );

        auto light_data = LightData{
            .direction = directional_light.direction,
            .color = directional_light.color,
            .view_position = camera_position,
            .screen_size = Maths::Vector4f{
                static_cast<float>(swapchain->width),
                static_cast<float>(swapchain->height),
                0.0F,
                0.0F,
            },
            .light_space_matrix = shadow_pass.get_light_space_matrix(),
            .shadow_params = Maths::Vector4f{
                static_cast<float>(
                    shadow_pass.get_shadow_map_texture().get_width()
                ),
                shadow_normal_offset_bias,
                0.0F,
                0.0F,
            },
            .cluster_params = Maths::Vector4f{
                camera_params.near_plane,
                camera_params.far_plane,
                0.0F,
                0.0F,
            },
        };

        mesh_render_pass.draw(
            *this->sdl_gpu_factory,
            command_buffer,
            render_pass,
            instance_batches,
            queued_draws,
            view_matrix * projection_matrix,
            camera_frustum_planes,
            instance_cull_pass.get_indirect_command_buffer(),
            instance_cull_pass.get_visible_instance_indices_buffer(),
            instance_cull_layout,
            light_data,
            ao_pass.get_ao_texture(),
            ao_pass.get_sampler(),
            shadow_pass.get_shadow_map_texture(),
            shadow_pass.get_sampler(),
            IBLTextures{
                .irradiance_texture = &ibl_render_pass.get_irradiance_texture(),
                .irradiance_sampler = &ibl_render_pass.get_irradiance_sampler(),
                .prefiltered_texture = &ibl_render_pass.get_prefiltered_texture(),
                .prefiltered_sampler = &ibl_render_pass.get_prefiltered_sampler(),
                .prefiltered_mip_count =
                    ibl_render_pass.get_prefiltered_mip_count(),
                .brdf_lut_texture = &ibl_render_pass.get_brdf_lut_texture(),
                .brdf_lut_sampler = &ibl_render_pass.get_brdf_lut_sampler(),
            },
            ClusteredLightBuffers{
                .point_lights = &cluster_pass.get_point_light_buffer(),
                .spot_lights = &cluster_pass.get_spot_light_buffer(),
                .cluster_light_grid = &cluster_pass.get_cluster_light_grid_buffer(),
                .global_light_index_list =
                    &cluster_pass.get_global_light_index_list_buffer(),
            },
            PointSpotShadowTextures{
                .point_shadow_texture =
                    &point_spot_shadow_pass.get_point_shadow_texture(),
                .point_shadow_sampler =
                    &point_spot_shadow_pass.get_point_shadow_sampler(),
                .spot_shadow_texture =
                    &point_spot_shadow_pass.get_spot_shadow_texture(),
                .spot_shadow_sampler =
                    &point_spot_shadow_pass.get_spot_shadow_sampler(),
                .spot_shadow_matrices =
                    &point_spot_shadow_pass.get_spot_shadow_matrix_buffer(),
            },
            ssr_pass.get_ssr_texture(),
            ssr_pass.get_sampler()
        );

        skybox_render_pass.draw(
            command_buffer, render_pass, view_matrix, projection_matrix
        );

        performance_logger.record(
            "main_pass", Units::Seconds{pass_timer.elapsed_seconds()}
        );
    }

    {
        const auto pass_timer = Utilities::Timer{};

        const auto tonemap_color_targets = std::array{ColorTargetInfo{
            .texture = &swapchain->texture,
            .load_op = LoadOp::DontCare,
            .store_op = StoreOp::Store,
        }};

        auto tonemap_render_pass =
            command_buffer.begin_render_pass(tonemap_color_targets);

        tonemap_pass.draw(
            command_buffer, tonemap_render_pass, hdr_color_texture, exposure
        );

        text_render_pass.draw(
            command_buffer,
            tonemap_render_pass,
            swapchain->width,
            swapchain->height
        );

        performance_logger.record(
            "tonemap", Units::Seconds{pass_timer.elapsed_seconds()}
        );
    }

    command_buffer.submit();
    queued_draws.clear();

    has_valid_previous_depth = true;

    // Ping-pong the HDR targets: this frame's resolved color becomes next
    // frame's SSR reflection source. Swapping the wrapper handles avoids a
    // per-frame texture copy; the tonemap pass above already consumed
    // hdr_color_texture, so it's safe to repurpose it as "previous" now.
    std::swap(hdr_color_texture, previous_hdr_color_texture);
    has_valid_previous_hdr = true;

    performance_logger.record(
        "frame", Units::Seconds{frame_timer.elapsed_seconds()}
    );
    performance_logger.end_frame();
}

auto SDL_GPURenderer::set_debug_disable_occlusion_culling(bool disabled)
    -> void {
    debug_disable_occlusion_culling = disabled;
}

auto SDL_GPURenderer::set_debug_visualize_hiz(bool enabled) -> void {
    debug_visualize_hiz = enabled;
}

auto SDL_GPURenderer::debug_log_visible_instance_count() -> void {
    const auto& indirect_buffer = instance_cull_pass.get_indirect_command_buffer();
    const auto buffer_size = indirect_buffer.get_size();
    if (buffer_size == 0) {
        SDL_Log("[OcclusionDebug] no indirect commands recorded yet");
        return;
    }

    // Debug-only readback: forces a full GPU sync twice, never do this on a
    // per-frame basis (see GPUDevice::wait_for_idle's doc comment).
    gpu_device->wait_for_idle();

    auto download_buffer = gpu_device->create_transfer_buffer(TransferBufferInfo{
        .usage = TransferBufferUsage::Download,
        .size = buffer_size,
    });

    {
        auto command_buffer = gpu_device->create_command_buffer();
        {
            auto copy_pass = command_buffer.begin_copy_pass();
            copy_pass.download_from_buffer(
                indirect_buffer, 0, download_buffer, 0, buffer_size
            );
        }
        command_buffer.submit();
    }

    gpu_device->wait_for_idle();

    const auto mapped = download_buffer.map(false);
    const auto command_count =
        static_cast<uint32_t>(buffer_size / sizeof(IndirectDrawCommand));
    auto total_visible_instances = uint32_t{0};
    for (auto i = uint32_t{0}; i < command_count; ++i) {
        auto command = IndirectDrawCommand{};
        std::memcpy(
            &command, mapped.data() + (i * sizeof(IndirectDrawCommand)),
            sizeof(IndirectDrawCommand)
        );
        total_visible_instances += command.num_instances;
    }
    download_buffer.unmap();

    SDL_Log(
        "[OcclusionDebug] occlusion_disabled=%d visible_instances=%u "
        "(across %u indirect commands)",
        debug_disable_occlusion_culling ? 1 : 0, total_visible_instances,
        command_count
    );
}

auto SDL_GPURenderer::queue_draw_text(
    FontId font_id,
    std::string_view text,
    const Maths::Vector2f& position,
    const Maths::Vector4f& color
) -> void {
    text_render_pass.queue_draw(
        *gpu_device, font_id, sdl_gpu_factory->get_font(font_id), text,
        position, color
    );
}

}  // namespace Luminol::Graphics::SDL_GPU
