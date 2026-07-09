#include "SDL_GPUIBLRenderPass.hpp"

#include <array>

#include <gsl/gsl>
#include <LuminolMaths/Transform.hpp>
#include <LuminolMaths/Units/Angle.hpp>
#include <LuminolMaths/Vector.hpp>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCommandBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUDevice.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPURenderPass.hpp>

namespace {

using namespace Luminol::Graphics::SDL_GPU;
using namespace Luminol::Maths;

constexpr auto ibl_texture_format = TextureFormat::R16G16B16A16_Float;

constexpr auto irradiance_size = uint32_t{32};
constexpr auto prefiltered_base_size = uint32_t{128};
constexpr auto default_prefiltered_mip_count = uint32_t{5};
constexpr auto brdf_lut_size = uint32_t{256};

constexpr auto cube_face_count = uint32_t{6};

struct CubeFace {
    Vector3f target;
    Vector3f up;
};

// Standard cubemap face capture basis, matching the +X,-X,+Y,-Y,+Z,-Z face
// order already used for the skybox's layer 0-5 upload
// (SDL_GPUSkybox.cpp).
constexpr auto cube_faces = std::array{
    CubeFace{.target = Vector3f{1.0f, 0.0f, 0.0f}, .up = Vector3f{0.0f, 1.0f, 0.0f}},
    CubeFace{.target = Vector3f{-1.0f, 0.0f, 0.0f}, .up = Vector3f{0.0f, 1.0f, 0.0f}},
    CubeFace{.target = Vector3f{0.0f, 1.0f, 0.0f}, .up = Vector3f{0.0f, 0.0f, -1.0f}},
    CubeFace{.target = Vector3f{0.0f, -1.0f, 0.0f}, .up = Vector3f{0.0f, 0.0f, 1.0f}},
    CubeFace{.target = Vector3f{0.0f, 0.0f, 1.0f}, .up = Vector3f{0.0f, 1.0f, 0.0f}},
    CubeFace{.target = Vector3f{0.0f, 0.0f, -1.0f}, .up = Vector3f{0.0f, 1.0f, 0.0f}},
};

auto cube_face_inv_view_proj(uint32_t face) -> Matrix4x4f {
    const auto view = Transform::left_handed_look_at_matrix(
        Transform::LookAtParams<float>{
            .eye = Vector3f{0.0f, 0.0f, 0.0f},
            .target = gsl::at(cube_faces, face).target,
            .up_vector = gsl::at(cube_faces, face).up,
        }
    );

    const auto projection = Transform::left_handed_perspective_projection_matrix(
        Transform::PerspectiveMatrixParams<float>{
            .fov = Luminol::Units::Degrees_f{90.0f},
            .aspect_ratio = 1.0f,
            .near_plane = 0.1f,
            .far_plane = 10.0f,
        }
    );

    return (view * projection).inverse();
}

auto make_shader(
    GPUDevice& device,
    const std::filesystem::path& path,
    ShaderStage stage,
    uint32_t sampler_count,
    uint32_t uniform_buffer_count
) -> Shader {
    return device.create_shader(ShaderInfo{
        .path = path,
        .stage = stage,
        .source_language = ShaderSourceLanguage::Hlsl,
        .sampler_count = sampler_count,
        .uniform_buffer_count = uniform_buffer_count,
        .storage_buffer_count = 0U,
    });
}

auto make_cube_pipeline(
    GPUDevice& device, const Shader& vertex_shader, const Shader& fragment_shader
) -> GraphicsPipeline {
    return device.create_graphics_pipeline(GraphicsPipelineInfo{
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_target_format = ibl_texture_format,
        .primitive_type = PrimitiveType::TriangleList,
        .vertex_buffer_descriptions = {},
        .vertex_attributes = {},
        .enable_depth_test = false,
        .cull_mode = CullMode::None,
    });
}

auto make_cube_texture(GPUDevice& device, uint32_t size, uint32_t mip_levels)
    -> Texture {
    return device.create_texture(TextureInfo{
        .width = size,
        .height = size,
        .format = ibl_texture_format,
        .usage = TextureUsage::ColorTarget | TextureUsage::Sampler,
        .type = TextureType::TextureCube,
        .mip_levels = mip_levels,
    });
}

auto make_clamp_sampler(GPUDevice& device, bool enable_mipmap_filtering)
    -> Sampler {
    return device.create_sampler(SamplerInfo{
        .filter = SamplerFilter::Linear,
        .address_mode_u = SamplerAddressMode::ClampToEdge,
        .address_mode_v = SamplerAddressMode::ClampToEdge,
        .enable_mipmap_filtering = enable_mipmap_filtering,
    });
}

}  // namespace

namespace Luminol::Graphics::SDL_GPU {

SDL_GPUIBLRenderPass::SDL_GPUIBLRenderPass(
    GPUDevice& device,
    const Texture& skybox_texture,
    const Sampler& skybox_sampler
)
    : cubemap_face_vertex_shader{make_shader(
          device,
          "res/shaders/sdl_gpu/skybox_vert.hlsl",
          ShaderStage::Vertex,
          0U,
          1U
      )},
      irradiance_fragment_shader{make_shader(
          device,
          "res/shaders/sdl_gpu/irradiance_convolve_frag.hlsl",
          ShaderStage::Fragment,
          1U,
          0U
      )},
      prefilter_fragment_shader{make_shader(
          device,
          "res/shaders/sdl_gpu/prefilter_specular_frag.hlsl",
          ShaderStage::Fragment,
          1U,
          1U
      )},
      irradiance_pipeline{make_cube_pipeline(
          device, cubemap_face_vertex_shader, irradiance_fragment_shader
      )},
      prefilter_pipeline{make_cube_pipeline(
          device, cubemap_face_vertex_shader, prefilter_fragment_shader
      )},
      fullscreen_vertex_shader{make_shader(
          device,
          "res/shaders/sdl_gpu/fullscreen_vert.hlsl",
          ShaderStage::Vertex,
          0U,
          0U
      )},
      brdf_lut_fragment_shader{make_shader(
          device,
          "res/shaders/sdl_gpu/brdf_lut_frag.hlsl",
          ShaderStage::Fragment,
          0U,
          0U
      )},
      brdf_lut_pipeline{device.create_graphics_pipeline(GraphicsPipelineInfo{
          .vertex_shader = fullscreen_vertex_shader,
          .fragment_shader = brdf_lut_fragment_shader,
          .color_target_format = ibl_texture_format,
          .primitive_type = PrimitiveType::TriangleList,
          .vertex_buffer_descriptions = {},
          .vertex_attributes = {},
          .enable_depth_test = false,
          .cull_mode = CullMode::None,
      })},
      irradiance_texture{make_cube_texture(device, irradiance_size, 1U)},
      irradiance_sampler{make_clamp_sampler(device, false)},
      prefiltered_texture{make_cube_texture(
          device, prefiltered_base_size, default_prefiltered_mip_count
      )},
      prefiltered_sampler{make_clamp_sampler(device, true)},
      prefiltered_mip_count{default_prefiltered_mip_count},
      brdf_lut_texture{device.create_texture(TextureInfo{
          .width = brdf_lut_size,
          .height = brdf_lut_size,
          .format = ibl_texture_format,
          .usage = TextureUsage::ColorTarget | TextureUsage::Sampler,
      })},
      brdf_lut_sampler{make_clamp_sampler(device, false)} {
    const auto irradiance_texture_view =
        TextureView{irradiance_texture.native_handle()};
    const auto prefiltered_texture_view =
        TextureView{prefiltered_texture.native_handle()};
    const auto brdf_lut_texture_view =
        TextureView{brdf_lut_texture.native_handle()};

    const auto skybox_sampler_bindings = std::array{TextureSamplerBinding{
        .texture = &skybox_texture, .sampler = &skybox_sampler
    }};

    auto command_buffer = device.create_command_buffer();

    for (auto face = uint32_t{0}; face < cube_face_count; ++face) {
        const auto inv_view_proj = cube_face_inv_view_proj(face);

        const auto color_targets = std::array{ColorTargetInfo{
            .texture = &irradiance_texture_view,
            .load_op = LoadOp::Clear,
            .store_op = StoreOp::Store,
            .mip_level = 0,
            .layer = face,
        }};

        auto render_pass = command_buffer.begin_render_pass(color_targets);
        render_pass.bind_graphics_pipeline(irradiance_pipeline);
        command_buffer.push_vertex_uniform_data(
            0,
            gsl::span{
                reinterpret_cast<const std::byte*>(&inv_view_proj),
                sizeof(inv_view_proj)
            }
        );
        render_pass.bind_fragment_samplers(0, skybox_sampler_bindings);
        render_pass.draw_primitives(3, 1, 0, 0);
    }

    for (auto mip = uint32_t{0}; mip < default_prefiltered_mip_count; ++mip) {
        const auto roughness = static_cast<float>(mip) /
            static_cast<float>(default_prefiltered_mip_count - 1);

        for (auto face = uint32_t{0}; face < cube_face_count; ++face) {
            const auto inv_view_proj = cube_face_inv_view_proj(face);

            const auto color_targets = std::array{ColorTargetInfo{
                .texture = &prefiltered_texture_view,
                .load_op = LoadOp::Clear,
                .store_op = StoreOp::Store,
                .mip_level = mip,
                .layer = face,
            }};

            auto render_pass = command_buffer.begin_render_pass(color_targets);
            render_pass.bind_graphics_pipeline(prefilter_pipeline);
            command_buffer.push_vertex_uniform_data(
                0,
                gsl::span{
                    reinterpret_cast<const std::byte*>(&inv_view_proj),
                    sizeof(inv_view_proj)
                }
            );
            command_buffer.push_fragment_uniform_data(
                0,
                gsl::span{
                    reinterpret_cast<const std::byte*>(&roughness),
                    sizeof(roughness)
                }
            );
            render_pass.bind_fragment_samplers(0, skybox_sampler_bindings);
            render_pass.draw_primitives(3, 1, 0, 0);
        }
    }

    {
        const auto color_targets = std::array{ColorTargetInfo{
            .texture = &brdf_lut_texture_view,
            .load_op = LoadOp::Clear,
            .store_op = StoreOp::Store,
        }};

        auto render_pass = command_buffer.begin_render_pass(color_targets);
        render_pass.bind_graphics_pipeline(brdf_lut_pipeline);
        render_pass.draw_primitives(3, 1, 0, 0);
    }

    command_buffer.submit();
}

auto SDL_GPUIBLRenderPass::get_irradiance_texture() const -> const Texture& {
    return irradiance_texture;
}

auto SDL_GPUIBLRenderPass::get_irradiance_sampler() const -> const Sampler& {
    return irradiance_sampler;
}

auto SDL_GPUIBLRenderPass::get_prefiltered_texture() const -> const Texture& {
    return prefiltered_texture;
}

auto SDL_GPUIBLRenderPass::get_prefiltered_sampler() const -> const Sampler& {
    return prefiltered_sampler;
}

auto SDL_GPUIBLRenderPass::get_prefiltered_mip_count() const -> uint32_t {
    return prefiltered_mip_count;
}

auto SDL_GPUIBLRenderPass::get_brdf_lut_texture() const -> const Texture& {
    return brdf_lut_texture;
}

auto SDL_GPUIBLRenderPass::get_brdf_lut_sampler() const -> const Sampler& {
    return brdf_lut_sampler;
}

}  // namespace Luminol::Graphics::SDL_GPU
