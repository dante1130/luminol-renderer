#include "OpenGLGBufferRenderPass.hpp"

#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLUniforms.hpp>

namespace {

using namespace Luminol::Graphics;

auto create_gbuffer_shader() -> OpenGLShader {
    auto gbuffer_shader = OpenGLShader{ShaderPaths{
        .vertex_shader_path =
            std::filesystem::path{"res/shaders/gbuffer_vert.glsl"},
        .fragment_shader_path =
            std::filesystem::path{"res/shaders/gbuffer_frag.glsl"},
        .geometry_shader_path = std::nullopt,
        .tessellation_control_shader_path = std::nullopt,
        .tessellation_evaluation_shader_path = std::nullopt,
        .compute_shader_path = std::nullopt,
    }};

    gbuffer_shader.bind();
    gbuffer_shader.set_uniform_block_binding_point(
        "Transform", UniformBufferBindingPoint::Transform
    );
    gbuffer_shader.set_shader_storage_block_binding_point(
        "InstancingModelMatrix",
        ShaderStorageBufferBindingPoint::InstancingModelMatrices
    );
    gbuffer_shader.set_sampler_binding_point(
        "material.texture_diffuse", SamplerBindingPoint::TextureDiffuse
    );
    gbuffer_shader.set_sampler_binding_point(
        "material.texture_emissive", SamplerBindingPoint::TextureEmissive
    );
    gbuffer_shader.set_sampler_binding_point(
        "material.texture_normal", SamplerBindingPoint::TextureNormal
    );
    gbuffer_shader.set_sampler_binding_point(
        "material.texture_metallic", SamplerBindingPoint::TextureMetallic
    );
    gbuffer_shader.set_sampler_binding_point(
        "material.texture_roughness", SamplerBindingPoint::TextureRoughness
    );
    gbuffer_shader.set_sampler_binding_point(
        "material.texture_ao", SamplerBindingPoint::TextureAO
    );
    gbuffer_shader.unbind();

    return gbuffer_shader;
}

auto create_geometry_frame_buffer(int32_t width, int32_t height)
    -> OpenGLFrameBuffer {
    return OpenGLFrameBuffer{OpenGLFrameBufferDescriptor{
        .width = width,
        .height = height,
        .color_attachments =
            {
                OpenGLFrameBufferAttachment{
                    .internal_format = TextureInternalFormat::RGBA16F,
                    .format = TextureFormat::RGBA,
                    .binding_point =
                        SamplerBindingPoint::GBufferPositionMetallic,
                },
                OpenGLFrameBufferAttachment{
                    .internal_format = TextureInternalFormat::RGBA16F,
                    .format = TextureFormat::RGBA,
                    .binding_point =
                        SamplerBindingPoint::GBufferNormalRoughness,
                },
                OpenGLFrameBufferAttachment{
                    .internal_format = TextureInternalFormat::RGBA16F,
                    .format = TextureFormat::RGBA,
                    .binding_point = SamplerBindingPoint::GBufferEmissiveAO,
                },
                OpenGLFrameBufferAttachment{
                    .internal_format = TextureInternalFormat::RGB8,
                    .format = TextureFormat::RGB,
                    .binding_point = SamplerBindingPoint::GBufferAlbedo,
                },
            },
    }};
}

}  // namespace

namespace Luminol::Graphics {

OpenGLGBufferRenderPass::OpenGLGBufferRenderPass(int32_t width, int32_t height)
    : gbuffer_frame_buffer{create_geometry_frame_buffer(width, height)},
      gbuffer_shader{create_gbuffer_shader()} {}

auto OpenGLGBufferRenderPass::get_gbuffer_frame_buffer() -> OpenGLFrameBuffer& {
    return this->gbuffer_frame_buffer;
}

auto OpenGLGBufferRenderPass::draw(
    const RenderableManager& renderable_manager,
    gsl::span<InstancedDrawCall> draw_calls,
    OpenGLShaderStorageBuffer& instancing_model_matrix_buffer
) const -> void {
    this->gbuffer_shader.bind();
    this->gbuffer_frame_buffer.bind();
    for (const auto& draw_call : draw_calls) {
        instancing_model_matrix_buffer.set_data(
            0,
            gsl::narrow<int64_t>(
                draw_call.model_matrices.size() * sizeof(Maths::Matrix4x4f)
            ),
            draw_call.model_matrices.data()
        );

        const auto& renderable =
            renderable_manager.get_renderable(draw_call.renderable_id);

        renderable.draw_instanced(
            gsl::narrow<int32_t>(draw_call.model_matrices.size())
        );
    }
    this->gbuffer_frame_buffer.unbind();
    this->gbuffer_shader.unbind();
}

}  // namespace Luminol::Graphics
