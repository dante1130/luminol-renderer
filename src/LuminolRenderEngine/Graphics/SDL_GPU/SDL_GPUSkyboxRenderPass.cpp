#include "SDL_GPUSkyboxRenderPass.hpp"

#include <array>
#include <filesystem>
#include <optional>

#include <gsl/gsl>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCommandBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCopyPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUDevice.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPURenderPass.hpp>

namespace {

using namespace Luminol::Graphics::SDL_GPU;
using namespace Luminol::Maths;

constexpr auto depth_texture_format = TextureFormat::D24_Unorm;
constexpr auto hdr_color_texture_format = TextureFormat::R16G16B16A16_Float;

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

auto make_skybox_pipeline(
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
        .vertex_buffer_descriptions = {},
        .vertex_attributes = {},
        .enable_depth_test = true,
        .enable_depth_write = false,
        .depth_stencil_format = depth_texture_format,
        .cull_mode = CullMode::None,
        .sample_count = sample_count,
    });
}

auto make_skybox(GPUDevice& device) -> SDL_GPUSkybox {
    const auto paths = Luminol::Graphics::SkyboxPaths{
        .front = std::filesystem::path{"res/skybox/default/front.jpg"},
        .back = std::filesystem::path{"res/skybox/default/back.jpg"},
        .top = std::filesystem::path{"res/skybox/default/top.jpg"},
        .bottom = std::filesystem::path{"res/skybox/default/bottom.jpg"},
        .left = std::filesystem::path{"res/skybox/default/left.jpg"},
        .right = std::filesystem::path{"res/skybox/default/right.jpg"},
    };

    auto command_buffer = device.create_command_buffer();
    auto skybox = std::optional<SDL_GPUSkybox>{};
    {
        auto copy_pass = command_buffer.begin_copy_pass();
        skybox.emplace(device, copy_pass, paths);
    }
    command_buffer.submit();

    return std::move(*skybox);
}

// Removes translation from the view matrix by stripping the last column of
// the top-left 3x3 block, so the skybox rotates with the camera but never
// translates.
auto strip_translation(const Matrix4x4f& view_matrix) -> Matrix4x4f {
    return Matrix4x4f{std::array{
        std::array{
            view_matrix[0][0], view_matrix[0][1], view_matrix[0][2], 0.0f
        },
        std::array{
            view_matrix[1][0], view_matrix[1][1], view_matrix[1][2], 0.0f
        },
        std::array{
            view_matrix[2][0], view_matrix[2][1], view_matrix[2][2], 0.0f
        },
        std::array{0.0f, 0.0f, 0.0f, 1.0f},
    }};
}

}  // namespace

namespace Luminol::Graphics::SDL_GPU {

SDL_GPUSkyboxRenderPass::SDL_GPUSkyboxRenderPass(
    GPUDevice& device, SampleCount sample_count
)
    : skybox_vertex_shader{make_shader(
          device,
          "res/shaders/sdl_gpu/skybox_vert.hlsl",
          ShaderStage::Vertex,
          0U,
          1U
      )},
      skybox_fragment_shader{make_shader(
          device,
          "res/shaders/sdl_gpu/skybox_frag.hlsl",
          ShaderStage::Fragment,
          1U,
          0U
      )},
      skybox_pipeline{make_skybox_pipeline(
          device, skybox_vertex_shader, skybox_fragment_shader, sample_count
      )},
      skybox{make_skybox(device)} {}

auto SDL_GPUSkyboxRenderPass::draw(
    CommandBuffer& command_buffer,
    RenderPass& render_pass,
    const Maths::Matrix4x4f& view_matrix,
    const Maths::Matrix4x4f& projection_matrix
) const -> void {
    const auto inv_view_proj =
        (strip_translation(view_matrix) * projection_matrix).inverse();

    render_pass.bind_graphics_pipeline(skybox_pipeline);

    command_buffer.push_vertex_uniform_data(
        0,
        gsl::span{
            reinterpret_cast<const std::byte*>(&inv_view_proj),
            sizeof(inv_view_proj)
        }
    );

    const auto sampler_bindings = std::array{TextureSamplerBinding{
        .texture = &skybox.get_texture(), .sampler = &skybox.get_sampler()
    }};
    render_pass.bind_fragment_samplers(0, sampler_bindings);

    render_pass.draw_primitives(3, 1, 0, 0);
}

auto SDL_GPUSkyboxRenderPass::get_skybox_texture() const -> const Texture& {
    return skybox.get_texture();
}

auto SDL_GPUSkyboxRenderPass::get_skybox_sampler() const -> const Sampler& {
    return skybox.get_sampler();
}

}  // namespace Luminol::Graphics::SDL_GPU
