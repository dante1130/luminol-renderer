#include "OpenGLRenderer.hpp"

#include <iostream>

#include <gsl/gsl>
#include <glad/gl.h>

#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLError.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLShader.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLBufferBit.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLUniforms.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLUniformBindingPoints.hpp>

namespace {

using namespace Luminol::Graphics;

auto initialize_opengl(
    Luminol::Window& window,
    const Luminol::Window::FramebufferSizeCallback& framebuffer_size_callback
) -> int32_t {
    const auto version = gladLoadGL(window.get_proc_address());
    Ensures(version != 0);

    std::cout << "OpenGL Version " << GLAD_VERSION_MAJOR(version) << "."
              << GLAD_VERSION_MINOR(version) << " loaded\n";

    window.set_framebuffer_size_callback(framebuffer_size_callback);

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(gl_debug_message_callback, nullptr);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CW);

    return version;
}

auto create_hdr_frame_buffer(int32_t width, int32_t height)
    -> OpenGLFrameBuffer {
    return OpenGLFrameBuffer{OpenGLFrameBufferDescriptor{
        .width = width,
        .height = height,
        .color_attachments = {OpenGLFrameBufferAttachment{
            .internal_format = TextureInternalFormat::RGBA16F,
            .format = TextureFormat::RGBA,
            .binding_point = SamplerBindingPoint::HDRFramebuffer,
        }},
    }};
}

auto create_transform_uniform_buffer() -> OpenGLUniformBuffer {
    return OpenGLUniformBuffer{
        UniformBufferBindingPoint::Transform, sizeof(OpenGLUniforms::Transform)
    };
}

auto create_light_uniform_buffer() -> OpenGLUniformBuffer {
    const auto size_bytes = offsetof(Light, point_lights) +
                            (sizeof(AlignedPointLight) * max_point_lights) +
                            (sizeof(AlignedSpotLight) * max_spot_lights);

    return OpenGLUniformBuffer{
        UniformBufferBindingPoint::Light, gsl::narrow<int64_t>(size_bytes)
    };
}

}  // namespace

namespace Luminol::Graphics {

OpenGLRenderer::OpenGLRenderer(Window& window, GraphicsApi graphics_api)
    : Renderer{graphics_api},
      opengl_version{
          initialize_opengl(window, this->get_framebuffer_resize_callback())
      },
      get_window_width{[&window]() { return window.get_width(); }},
      get_window_height{[&window]() { return window.get_height(); }},
      hdr_frame_buffer{create_hdr_frame_buffer(
          this->get_window_width(), this->get_window_height()
      )},
      gbuffer_render_pass{this->get_window_width(), this->get_window_height()},
      auto_exposure_render_pass{
          this->get_window_width(), this->get_window_height()
      },
      transform_uniform_buffer{create_transform_uniform_buffer()},
      light_uniform_buffer{create_light_uniform_buffer()},
      instancing_model_matrix_buffer{
          ShaderStorageBufferBindingPoint::InstancingModelMatrices
      },
      instancing_color_buffer{ShaderStorageBufferBindingPoint::Color},
      view_matrix{Maths::Matrix4x4f::identity()},
      projection_matrix{Maths::Matrix4x4f::identity()} {}

auto OpenGLRenderer::set_view_matrix(const Maths::Matrix4x4f& view_matrix)
    -> void {
    this->view_matrix = view_matrix;
}

auto OpenGLRenderer::set_projection_matrix(
    const Maths::Matrix4x4f& projection_matrix
) -> void {
    this->projection_matrix = projection_matrix;
}

auto OpenGLRenderer::set_exposure(float exposure) -> void {
    this->exposure = exposure;
}

auto OpenGLRenderer::clear_color(const Maths::Vector4f& color) const -> void {
    glClearColor(color.x(), color.y(), color.z(), color.w());
}

auto OpenGLRenderer::clear(BufferBit buffer_bit) const -> void {
    glClear(buffer_bit_to_gl(buffer_bit));
}

auto OpenGLRenderer::queue_draw(
    RenderableId renderable_id, const Maths::Matrix4x4f& model_matrix
) -> void {
    if (this->instanced_draw_call_map.contains(renderable_id)) {
        auto& draw_call = this->instanced_draw_call_map.at(renderable_id);
        draw_call.model_matrices.emplace_back(model_matrix);
        return;
    }

    this->instanced_draw_queue.emplace_back(InstancedDrawCall{
        .renderable_id = renderable_id,
        .model_matrices = {model_matrix},
    });

    this->instanced_draw_call_map.emplace(
        renderable_id, this->instanced_draw_queue.back()
    );
}

auto OpenGLRenderer::queue_draw_with_color(
    RenderableId renderable_id,
    const Maths::Matrix4x4f& model_matrix,
    const Maths::Vector3f& color
) -> void {
    if (this->instanced_color_draw_call_map.contains(renderable_id)) {
        auto& draw_call = this->instanced_color_draw_call_map.at(renderable_id);
        draw_call.model_matrices.emplace_back(model_matrix);
        draw_call.colors.emplace_back(color.x(), color.y(), color.z(), 1.0f);
        return;
    }

    this->instanced_color_draw_queue.emplace_back(ColorInstancedDrawCall{
        .renderable_id = renderable_id,
        .model_matrices = {model_matrix},
        .colors = {Maths::Vector4f{color.x(), color.y(), color.z(), 1.0f}},
    });

    this->instanced_color_draw_call_map.emplace(
        renderable_id, this->instanced_color_draw_queue.back()
    );
}

auto OpenGLRenderer::queue_draw_line(
    const Maths::Vector3f& start_position,
    const Maths::Vector3f& end_position,
    const Maths::Vector3f& color
) -> void {
    this->line_draw_call.lines.emplace_back(LineDrawCall::Line{
        .start_position = start_position,
        .end_position = end_position,
    });

    this->line_draw_call.colors.emplace_back(
        color.x(), color.y(), color.z(), 1.0f
    );
}

auto OpenGLRenderer::draw() -> void {
    this->update_lights();

    const auto transform = OpenGLUniforms::Transform{
        .view_matrix = this->view_matrix,
        .projection_matrix = this->projection_matrix,
    };

    this->transform_uniform_buffer.set_data(
        0, sizeof(OpenGLUniforms::Transform), &transform
    );

    this->gbuffer_render_pass.get_gbuffer_frame_buffer().bind();
    this->clear(BufferBit::ColorDepth);
    this->gbuffer_render_pass.get_gbuffer_frame_buffer().unbind();

    this->gbuffer_render_pass.draw(
        this->get_renderable_manager(),
        this->instanced_draw_queue,
        this->instancing_model_matrix_buffer
    );

    this->hdr_frame_buffer.bind();
    this->clear(BufferBit::ColorDepth);
    this->hdr_frame_buffer.unbind();

    this->lighting_render_pass.draw(
        this->gbuffer_render_pass.get_gbuffer_frame_buffer(),
        this->hdr_frame_buffer,
        this->view_matrix
    );

    this->color_render_pass.draw(
        this->get_renderable_manager(),
        this->hdr_frame_buffer,
        this->instanced_color_draw_queue,
        this->instancing_model_matrix_buffer,
        this->instancing_color_buffer
    );

    if (!this->line_draw_call.lines.empty()) {
        this->primitive_render_pass.draw_lines(
            this->hdr_frame_buffer,
            this->line_draw_call,
            this->instancing_color_buffer
        );
    }

    this->skybox_render_pass.draw(
        this->hdr_frame_buffer,
        this->transform_uniform_buffer,
        this->view_matrix
    );

    constexpr float delta_time = 1.0f / 144.0f;
    this->auto_exposure_render_pass.draw(this->hdr_frame_buffer, delta_time);

    this->hdr_render_pass.draw(this->hdr_frame_buffer, this->exposure);

    this->instanced_draw_queue.clear();
    this->instanced_color_draw_queue.clear();
    this->instanced_draw_call_map.clear();
    this->instanced_color_draw_call_map.clear();

    this->line_draw_call.lines.clear();
    this->line_draw_call.colors.clear();
}

auto OpenGLRenderer::update_lights() -> void {
    const auto& light_data = this->get_light_manager().get_light_data();

    this->light_uniform_buffer.set_data(
        0, offsetof(Light, point_lights), &light_data
    );

    this->light_uniform_buffer.set_data(
        offsetof(Light, point_lights),
        gsl::narrow<int64_t>(
            sizeof(light_data.point_lights[0]) * light_data.point_lights.size()
        ),
        light_data.point_lights.data()
    );

    const auto spot_light_offset =
        offsetof(Light, point_lights) +
        (sizeof(light_data.point_lights[0]) * max_point_lights);

    this->light_uniform_buffer.set_data(
        gsl::narrow<int64_t>(spot_light_offset),
        gsl::narrow<int64_t>(
            sizeof(light_data.spot_lights[0]) * light_data.spot_lights.size()
        ),
        light_data.spot_lights.data()
    );
}

auto OpenGLRenderer::get_framebuffer_resize_callback()
    -> Window::FramebufferSizeCallback {
    return [this](int width, int height) {
        glViewport(0, 0, width, height);
        this->gbuffer_render_pass.get_gbuffer_frame_buffer().resize(
            width, height
        );
        this->hdr_frame_buffer.resize(width, height);
    };
}

}  // namespace Luminol::Graphics
