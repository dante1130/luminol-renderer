#include "SDL_GPUAmbientOcclusionPass.hpp"

#include <array>
#include <cstddef>

#include <SDL3/SDL_video.h>

#include <LuminolMaths/Vector.hpp>

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

constexpr auto depth_texture_format = TextureFormat::D24_Unorm;
constexpr auto ao_texture_format = TextureFormat::R8G8B8A8_Unorm;

struct SSAOUniforms {
    Matrix4x4f projection_matrix;
    Matrix4x4f inverse_projection_matrix;
    Vector4f params;
    Vector4f viewport_size;
};

struct BlurUniforms {
    Vector4f viewport_size;
};

auto make_ao_texture(GPUDevice& device, uint32_t width, uint32_t height)
    -> Texture {
    return device.create_texture(TextureInfo{
        .width = width,
        .height = height,
        .format = ao_texture_format,
        .usage = TextureUsage::ColorTarget | TextureUsage::Sampler,
    });
}

auto make_ao_texture(GPUDevice& device, SDL_Window* window) -> Texture {
    auto width = int{0};
    auto height = int{0};
    SDL_GetWindowSizeInPixels(window, &width, &height);

    return make_ao_texture(
        device, static_cast<uint32_t>(width), static_cast<uint32_t>(height)
    );
}

auto make_shader(
    GPUDevice& device,
    const std::filesystem::path& path,
    ShaderStage stage,
    uint32_t sampler_count,
    uint32_t uniform_buffer_count,
    uint32_t storage_buffer_count
) -> Shader {
    return device.create_shader(ShaderInfo{
        .path = path,
        .stage = stage,
        .source_language = ShaderSourceLanguage::Hlsl,
        .sampler_count = sampler_count,
        .uniform_buffer_count = uniform_buffer_count,
        .storage_buffer_count = storage_buffer_count,
    });
}

auto make_normal_prepass_pipeline(
    GPUDevice& device,
    const Shader& vertex_shader,
    const Shader& fragment_shader
) -> GraphicsPipeline {
    return device.create_graphics_pipeline(GraphicsPipelineInfo{
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_target_format = ao_texture_format,
        .primitive_type = PrimitiveType::TriangleList,
        .vertex_buffer_descriptions = mesh_vertex_buffer_descriptions,
        .vertex_attributes = mesh_vertex_attributes,
        .enable_depth_test = true,
        .depth_stencil_format = depth_texture_format,
        .cull_mode = CullMode::Back,
        .front_face = FrontFace::Clockwise,
    });
}

auto make_fullscreen_pipeline(
    GPUDevice& device,
    const Shader& vertex_shader,
    const Shader& fragment_shader
) -> GraphicsPipeline {
    return device.create_graphics_pipeline(GraphicsPipelineInfo{
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_target_format = ao_texture_format,
        .primitive_type = PrimitiveType::TriangleList,
        .vertex_buffer_descriptions = {},
        .vertex_attributes = {},
        .enable_depth_test = false,
        .cull_mode = CullMode::None,
    });
}

}  // namespace

namespace Luminol::Graphics::SDL_GPU {

SDL_GPUAmbientOcclusionPass::SDL_GPUAmbientOcclusionPass(
    GPUDevice& device, SDL_Window* window
)
    : normal_prepass_vertex_shader{make_shader(
          device,
          "res/shaders/sdl_gpu/pbr_vert.hlsl",
          ShaderStage::Vertex,
          0U,
          1U,
          1U
      )},
      normal_prepass_fragment_shader{make_shader(
          device,
          "res/shaders/sdl_gpu/normal_prepass_frag.hlsl",
          ShaderStage::Fragment,
          0U,
          1U,
          0U
      )},
      normal_prepass_pipeline{make_normal_prepass_pipeline(
          device, normal_prepass_vertex_shader, normal_prepass_fragment_shader
      )},
      fullscreen_vertex_shader{make_shader(
          device,
          "res/shaders/sdl_gpu/fullscreen_vert.hlsl",
          ShaderStage::Vertex,
          0U,
          0U,
          0U
      )},
      ssao_fragment_shader{make_shader(
          device,
          "res/shaders/sdl_gpu/ssao_frag.hlsl",
          ShaderStage::Fragment,
          2U,
          1U,
          0U
      )},
      ssao_pipeline{make_fullscreen_pipeline(
          device, fullscreen_vertex_shader, ssao_fragment_shader
      )},
      blur_fragment_shader{make_shader(
          device,
          "res/shaders/sdl_gpu/ssao_blur_frag.hlsl",
          ShaderStage::Fragment,
          1U,
          1U,
          0U
      )},
      blur_pipeline{make_fullscreen_pipeline(
          device, fullscreen_vertex_shader, blur_fragment_shader
      )},
      normal_texture{make_ao_texture(device, window)},
      ssao_raw_texture{make_ao_texture(device, window)},
      ssao_texture{make_ao_texture(device, window)},
      clamp_sampler{device.create_sampler(SamplerInfo{
          .filter = SamplerFilter::Linear,
          .address_mode_u = SamplerAddressMode::ClampToEdge,
          .address_mode_v = SamplerAddressMode::ClampToEdge,
      })} {}

auto SDL_GPUAmbientOcclusionPass::resize(
    GPUDevice& device, uint32_t width, uint32_t height
) -> void {
    normal_texture = make_ao_texture(device, width, height);
    ssao_raw_texture = make_ao_texture(device, width, height);
    ssao_texture = make_ao_texture(device, width, height);
}

auto SDL_GPUAmbientOcclusionPass::draw(
    const SDL_GPUFactory& graphics_factory,
    CommandBuffer& command_buffer,
    const SDL_GPUInstanceBufferCache& instance_buffer_cache,
    gsl::span<const InstanceBatch> instance_batches,
    const Maths::Matrix4x4f& view_matrix,
    const Maths::Matrix4x4f& projection_matrix,
    const Texture& depth_texture,
    Utilities::PerformanceLogger& performance_logger
) -> void {
    const auto depth_texture_view = TextureView{depth_texture.native_handle()};
    const auto normal_texture_view =
        TextureView{normal_texture.native_handle()};
    const auto ssao_raw_texture_view =
        TextureView{ssao_raw_texture.native_handle()};
    const auto ssao_texture_view = TextureView{ssao_texture.native_handle()};

    // Normal + depth prepass.
    {
        const auto pass_timer = Utilities::Timer{};

        const auto color_targets = std::array{ColorTargetInfo{
            .texture = &normal_texture_view,
            .clear_color = {0.5F, 0.5F, 1.0F, 1.0F},
            .load_op = LoadOp::Clear,
            .store_op = StoreOp::Store,
            .cycle = true,
        }};
        const auto depth_stencil_target = DepthStencilTargetInfo{
            .texture = &depth_texture_view,
            .clear_depth = 1.0F,
            .load_op = LoadOp::Clear,
            .store_op = StoreOp::Store,
            .cycle = true,
        };

        auto render_pass = command_buffer.begin_render_pass(
            color_targets, &depth_stencil_target
        );
        render_pass.bind_graphics_pipeline(normal_prepass_pipeline);

        const auto view_proj = view_matrix * projection_matrix;
        command_buffer.push_vertex_uniform_data(
            0,
            gsl::span{
                reinterpret_cast<const std::byte*>(&view_proj),
                sizeof(view_proj)
            }
        );
        command_buffer.push_fragment_uniform_data(
            0,
            gsl::span{
                reinterpret_cast<const std::byte*>(&view_matrix),
                sizeof(view_matrix)
            }
        );

        for (const auto& batch : instance_batches) {
            const auto& instance_buffer =
                instance_buffer_cache.get(batch.renderable_id);
            const auto storage_buffer_bindings = std::array{&instance_buffer};
            render_pass.bind_vertex_storage_buffers(
                0, storage_buffer_bindings
            );

            for (const auto& mesh :
                 graphics_factory.get_meshes(batch.renderable_id)) {
                mesh.draw_instanced_geometry_only(
                    static_cast<int32_t>(batch.instance_count), render_pass
                );
            }
        }

        performance_logger.record(
            "ao_normal_prepass", Units::Seconds{pass_timer.elapsed_seconds()}
        );
    }

    // SSAO pass.
    {
        const auto pass_timer = Utilities::Timer{};

        const auto color_targets = std::array{ColorTargetInfo{
            .texture = &ssao_raw_texture_view,
            .clear_color = {1.0F, 1.0F, 1.0F, 1.0F},
            .load_op = LoadOp::Clear,
            .store_op = StoreOp::Store,
            .cycle = true,
        }};

        auto render_pass = command_buffer.begin_render_pass(color_targets);
        render_pass.bind_graphics_pipeline(ssao_pipeline);

        const auto ssao_uniforms = SSAOUniforms{
            .projection_matrix = projection_matrix,
            .inverse_projection_matrix = projection_matrix.inverse(),
            .params = Vector4f{radius, bias, power, 0.0F},
            .viewport_size = Vector4f{
                static_cast<float>(normal_texture.get_width()),
                static_cast<float>(normal_texture.get_height()),
                0.0F,
                0.0F,
            },
        };
        command_buffer.push_fragment_uniform_data(
            0,
            gsl::span{
                reinterpret_cast<const std::byte*>(&ssao_uniforms),
                sizeof(ssao_uniforms)
            }
        );

        const auto sampler_bindings = std::array{
            TextureSamplerBinding{
                .texture = &depth_texture, .sampler = &clamp_sampler
            },
            TextureSamplerBinding{
                .texture = &normal_texture, .sampler = &clamp_sampler
            },
        };
        render_pass.bind_fragment_samplers(0, sampler_bindings);

        render_pass.draw_primitives(3, 1, 0, 0);

        performance_logger.record(
            "ao_ssao", Units::Seconds{pass_timer.elapsed_seconds()}
        );
    }

    // Blur pass.
    {
        const auto pass_timer = Utilities::Timer{};

        const auto color_targets = std::array{ColorTargetInfo{
            .texture = &ssao_texture_view,
            .clear_color = {1.0F, 1.0F, 1.0F, 1.0F},
            .load_op = LoadOp::Clear,
            .store_op = StoreOp::Store,
            .cycle = true,
        }};

        auto render_pass = command_buffer.begin_render_pass(color_targets);
        render_pass.bind_graphics_pipeline(blur_pipeline);

        const auto blur_uniforms = BlurUniforms{
            .viewport_size = Vector4f{
                static_cast<float>(ssao_raw_texture.get_width()),
                static_cast<float>(ssao_raw_texture.get_height()),
                0.0F,
                0.0F,
            },
        };
        command_buffer.push_fragment_uniform_data(
            0,
            gsl::span{
                reinterpret_cast<const std::byte*>(&blur_uniforms),
                sizeof(blur_uniforms)
            }
        );

        const auto sampler_bindings = std::array{TextureSamplerBinding{
            .texture = &ssao_raw_texture, .sampler = &clamp_sampler
        }};
        render_pass.bind_fragment_samplers(0, sampler_bindings);

        render_pass.draw_primitives(3, 1, 0, 0);

        performance_logger.record(
            "ao_blur", Units::Seconds{pass_timer.elapsed_seconds()}
        );
    }
}

auto SDL_GPUAmbientOcclusionPass::get_ao_texture() const -> const Texture& {
    return ssao_texture;
}

auto SDL_GPUAmbientOcclusionPass::get_sampler() const -> const Sampler& {
    return clamp_sampler;
}

}  // namespace Luminol::Graphics::SDL_GPU
