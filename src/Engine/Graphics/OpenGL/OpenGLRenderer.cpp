#include "OpenGLRenderer.hpp"

#include <iostream>

#include <gsl/gsl>
#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>

#include <Engine/Graphics/OpenGL/OpenGLError.hpp>
#include <Engine/Graphics/OpenGL/OpenGLShader.hpp>

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

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CW);

    return version;
}

constexpr auto buffer_bit_to_gl(BufferBit buffer_bit) -> GLenum {
    switch (buffer_bit) {
        case BufferBit::Color:
            return GL_COLOR_BUFFER_BIT;
        case BufferBit::Depth:
            return GL_DEPTH_BUFFER_BIT;
        case BufferBit::Stencil:
            return GL_STENCIL_BUFFER_BIT;
        case BufferBit::ColorDepth:
            return GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
        case BufferBit::ColorStencil:
            return GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
        case BufferBit::DepthStencil:
            return GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
        case BufferBit::ColorDepthStencil:
            return GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
                   GL_STENCIL_BUFFER_BIT;
    }

    return 0;
}

auto create_quad_mesh() -> OpenGLMesh {
    struct Vertex {
        glm::vec3 position;
        glm::vec2 tex_coords;
        glm::vec3 normal;
        glm::vec3 tangent;
    };

    constexpr auto vertices = std::array{
        Vertex{
            .position = glm::vec3{-1.0f, 1.0f, 0.0f},
            .tex_coords = glm::vec2{0.0f, 1.0f},
            .normal = glm::vec3{0.0f, 0.0f, 1.0f},
            .tangent = glm::vec3{1.0f, 0.0f, 0.0f}
        },
        Vertex{
            .position = glm::vec3{-1.0f, -1.0f, 0.0f},
            .tex_coords = glm::vec2{0.0f, 0.0f},
            .normal = glm::vec3{0.0f, 0.0f, 1.0f},
            .tangent = glm::vec3{1.0f, 0.0f, 0.0f}
        },
        Vertex{
            .position = glm::vec3{1.0f, -1.0f, 0.0f},
            .tex_coords = glm::vec2{1.0f, 0.0f},
            .normal = glm::vec3{0.0f, 0.0f, 1.0f},
            .tangent = glm::vec3{1.0f, 0.0f, 0.0f}
        },
        Vertex{
            .position = glm::vec3{1.0f, 1.0f, 0.0f},
            .tex_coords = glm::vec2{1.0f, 1.0f},
            .normal = glm::vec3{0.0f, 0.0f, 1.0f},
            .tangent = glm::vec3{1.0f, 0.0f, 0.0f}
        }
    };

    // Draw in counter-clockwise order
    constexpr auto indices = std::array{0u, 3u, 2u, 2u, 1u, 0u};

    constexpr auto component_count = 11u;

    auto vertices_float = std::vector<float>{};
    vertices_float.reserve(vertices.size() * component_count);

    for (const auto& vertex : vertices) {
        vertices_float.push_back(vertex.position.x);
        vertices_float.push_back(vertex.position.y);
        vertices_float.push_back(vertex.position.z);

        vertices_float.push_back(vertex.tex_coords.x);
        vertices_float.push_back(vertex.tex_coords.y);

        vertices_float.push_back(vertex.normal.x);
        vertices_float.push_back(vertex.normal.y);
        vertices_float.push_back(vertex.normal.z);

        vertices_float.push_back(vertex.tangent.x);
        vertices_float.push_back(vertex.tangent.y);
        vertices_float.push_back(vertex.tangent.z);
    }

    return OpenGLMesh{vertices_float, indices};
}

auto create_color_shader() -> OpenGLShader {
    auto color_shader = OpenGLShader{ShaderPaths{
        .vertex_shader_path =
            std::filesystem::path{"res/shaders/color_vert.glsl"},
        .fragment_shader_path =
            std::filesystem::path{"res/shaders/color_frag.glsl"}
    }};

    color_shader.bind();
    color_shader.set_sampler_binding_point(
        "texture_diffuse", SamplerBindingPoint::TextureDiffuse
    );
    color_shader.set_uniform_block_binding_point(
        "Transform", UniformBufferBindingPoint::Transform
    );
    color_shader.unbind();

    return color_shader;
}

auto create_phong_shader() -> OpenGLShader {
    auto phong_shader = OpenGLShader{ShaderPaths{
        .vertex_shader_path =
            std::filesystem::path{"res/shaders/phong_vert.glsl"},
        .fragment_shader_path =
            std::filesystem::path{"res/shaders/phong_frag.glsl"}
    }};

    phong_shader.bind();
    phong_shader.set_sampler_binding_point(
        "gbuffer.position", SamplerBindingPoint::GBufferPosition
    );
    phong_shader.set_sampler_binding_point(
        "gbuffer.normal", SamplerBindingPoint::GBufferNormal
    );
    phong_shader.set_sampler_binding_point(
        "gbuffer.albedo_spec", SamplerBindingPoint::GBufferAlbedoSpec
    );
    phong_shader.set_sampler_binding_point(
        "skybox", SamplerBindingPoint::Skybox
    );
    phong_shader.set_uniform_block_binding_point(
        "Light", UniformBufferBindingPoint::Light
    );
    phong_shader.unbind();

    return phong_shader;
}

auto create_skybox_shader() -> OpenGLShader {
    auto skybox_shader = OpenGLShader{ShaderPaths{
        .vertex_shader_path =
            std::filesystem::path{"res/shaders/skybox_vert.glsl"},
        .fragment_shader_path =
            std::filesystem::path{"res/shaders/skybox_frag.glsl"}
    }};

    skybox_shader.bind();
    skybox_shader.set_sampler_binding_point(
        "skybox", SamplerBindingPoint::Skybox
    );
    skybox_shader.set_uniform_block_binding_point(
        "Transform", UniformBufferBindingPoint::Transform
    );
    skybox_shader.unbind();

    return skybox_shader;
}

auto create_hdr_shader() -> OpenGLShader {
    auto hdr_shader = OpenGLShader{ShaderPaths{
        .vertex_shader_path =
            std::filesystem::path{"res/shaders/hdr_vert.glsl"},
        .fragment_shader_path =
            std::filesystem::path{"res/shaders/hdr_frag.glsl"}
    }};

    hdr_shader.bind();
    hdr_shader.set_sampler_binding_point(
        "hdr_framebuffer", SamplerBindingPoint::HDRFramebuffer
    );
    hdr_shader.unbind();

    return hdr_shader;
}

auto create_gbuffer_shader() -> OpenGLShader {
    auto gbuffer_shader = OpenGLShader{ShaderPaths{
        .vertex_shader_path =
            std::filesystem::path{"res/shaders/gbuffer_vert.glsl"},
        .fragment_shader_path =
            std::filesystem::path{"res/shaders/gbuffer_frag.glsl"},
    }};

    gbuffer_shader.bind();
    gbuffer_shader.set_uniform_block_binding_point(
        "Transform", UniformBufferBindingPoint::Transform
    );
    gbuffer_shader.set_sampler_binding_point(
        "material.texture_diffuse", SamplerBindingPoint::TextureDiffuse
    );
    gbuffer_shader.set_sampler_binding_point(
        "material.texture_emissive", SamplerBindingPoint::TextureEmissive
    );
    gbuffer_shader.set_sampler_binding_point(
        "material.texture_specular", SamplerBindingPoint::TextureSpecular
    );
    gbuffer_shader.set_sampler_binding_point(
        "material.texture_normal", SamplerBindingPoint::TextureNormal
    );
    gbuffer_shader.unbind();

    return gbuffer_shader;
}

auto create_hdr_frame_buffer(int32_t width, int32_t height)
    -> OpenGLFrameBuffer {
    return OpenGLFrameBuffer{OpenGLFrameBufferDescriptor{
        width,
        height,
        {OpenGLFrameBufferAttachment{
            .internal_format = TextureInternalFormat::RGBA16F,
            .format = TextureFormat::RGBA,
            .binding_point = SamplerBindingPoint::HDRFramebuffer,
        }}
    }};
}

auto create_geometry_frame_buffer(int32_t width, int32_t height)
    -> OpenGLFrameBuffer {
    return OpenGLFrameBuffer{OpenGLFrameBufferDescriptor{
        width,
        height,
        {
            OpenGLFrameBufferAttachment{
                .internal_format = TextureInternalFormat::RGBA16F,
                .format = TextureFormat::RGBA,
                .binding_point = SamplerBindingPoint::GBufferPosition,
            },
            OpenGLFrameBufferAttachment{
                .internal_format = TextureInternalFormat::RGBA16F,
                .format = TextureFormat::RGBA,
                .binding_point = SamplerBindingPoint::GBufferNormal,
            },
            OpenGLFrameBufferAttachment{
                .internal_format = TextureInternalFormat::RGBA8,
                .format = TextureFormat::RGBA,
                .binding_point = SamplerBindingPoint::GBufferAlbedoSpec,
            },
        }
    }};
}

auto get_view_position(const glm::mat4& view_matrix) -> glm::vec3 {
    return glm::inverse(view_matrix)[3];
}

}  // namespace

namespace Luminol::Graphics {

OpenGLRenderer::OpenGLRenderer(Window& window)
    : opengl_version{initialize_opengl(
          window, this->get_framebuffer_resize_callback()
      )},
      get_window_width{[&window]() { return window.get_width(); }},
      get_window_height{[&window]() { return window.get_height(); }},
      color_shader{create_color_shader()},
      phong_shader{create_phong_shader()},
      skybox_shader{create_skybox_shader()},
      hdr_shader{create_hdr_shader()},
      gbuffer_shader{create_gbuffer_shader()},
      hdr_frame_buffer{create_hdr_frame_buffer(
          this->get_window_width(), this->get_window_height()
      )},
      geometry_frame_buffer{create_geometry_frame_buffer(
          this->get_window_width(), this->get_window_height()
      )},
      transform_uniform_buffer{OpenGLUniformBuffer<OpenGLUniforms::Transform>{
          OpenGLUniforms::Transform{}, UniformBufferBindingPoint::Transform
      }},
      light_uniform_buffer{OpenGLUniformBuffer<OpenGLUniforms::Light>{
          OpenGLUniforms::Light{}, UniformBufferBindingPoint::Light
      }},
      skybox{OpenGLSkybox{SkyboxPaths{
          .front = std::filesystem::path{"res/skybox/default/front.jpg"},
          .back = std::filesystem::path{"res/skybox/default/back.jpg"},
          .top = std::filesystem::path{"res/skybox/default/top.jpg"},
          .bottom = std::filesystem::path{"res/skybox/default/bottom.jpg"},
          .left = std::filesystem::path{"res/skybox/default/left.jpg"},
          .right = std::filesystem::path{"res/skybox/default/right.jpg"}
      }}},
      cube{"res/models/cube/cube.obj"},
      quad{create_quad_mesh()},
      view_matrix{glm::mat4{1.0f}},
      projection_matrix{glm::mat4{1.0f}} {}

auto OpenGLRenderer::set_view_matrix(const glm::mat4& view_matrix) -> void {
    this->view_matrix = view_matrix;
}

auto OpenGLRenderer::set_projection_matrix(const glm::mat4& projection_matrix)
    -> void {
    this->projection_matrix = projection_matrix;
}

auto OpenGLRenderer::set_exposure(float exposure) -> void {
    this->exposure = exposure;
}

auto OpenGLRenderer::clear_color(const glm::vec4& color) const -> void {
    glClearColor(color.r, color.g, color.b, color.a);
}

auto OpenGLRenderer::clear(BufferBit buffer_bit) const -> void {
    glClear(buffer_bit_to_gl(buffer_bit));
}

auto OpenGLRenderer::queue_draw(
    const Renderable& renderable, const glm::mat4& model_matrix
) -> void {
    this->draw_queue.emplace_back(renderable, model_matrix);
}

auto OpenGLRenderer::queue_draw_with_cell_shading(
    const Renderable& renderable,
    const glm::mat4& model_matrix,
    float cell_shading_levels
) -> void {
    Ensures(cell_shading_levels > 0);

    this->cell_shading_draw_queue.emplace_back(
        renderable, model_matrix, cell_shading_levels
    );
}

auto OpenGLRenderer::queue_draw_with_color(
    const Renderable& renderable,
    const glm::mat4& model_matrix,
    const glm::vec3& color
) -> void {
    this->color_draw_queue.emplace_back(renderable, model_matrix, color);
}

auto OpenGLRenderer::draw() -> void {
    this->update_lights();

    this->gbuffer_shader.bind();
    this->geometry_frame_buffer.bind();
    this->clear(BufferBit::ColorDepth);
    this->draw_skybox();
    this->draw_geometry();
    this->geometry_frame_buffer.unbind();

    this->hdr_frame_buffer.bind();
    this->clear(BufferBit::ColorDepth);
    this->draw_lighting();
    this->hdr_frame_buffer.unbind();

    this->hdr_shader.bind();
    this->clear(BufferBit::ColorDepth);
    this->hdr_shader.set_uniform("exposure", this->exposure);
    this->hdr_frame_buffer.bind_color_attachments();
    this->quad.get_render_command()();

    this->draw_queue.clear();
    this->color_draw_queue.clear();
    this->cell_shading_draw_queue.clear();
}

auto OpenGLRenderer::draw_geometry() -> void {
    const auto draw_with_transform = [this](const auto& draw_calls) {
        for (const auto& draw_call : draw_calls) {
            this->transform_uniform_buffer.set_data(OpenGLUniforms::Transform{
                .model_matrix = draw_call.model_matrix,
                .view_matrix = this->view_matrix,
                .projection_matrix = this->projection_matrix
            });

            draw_call.renderable.get().get_render_command()();
        }
    };

    draw_with_transform(this->draw_queue);
    draw_with_transform(this->cell_shading_draw_queue);
    draw_with_transform(this->color_draw_queue);
}

auto OpenGLRenderer::draw_lighting() -> void {
    for (const auto& draw_call : this->draw_queue) {
        const auto& renderable = draw_call.renderable.get();

        this->phong_shader.bind();
        this->geometry_frame_buffer.bind_color_attachments();
        this->phong_shader.set_uniform(
            "view_position", get_view_position(this->view_matrix)
        );
        this->phong_shader.set_uniform(
            "material.shininess", renderable.get_material().shininess
        );
        this->phong_shader.set_uniform("is_cell_shading", 0);

        this->quad.get_render_command()();
    }
}

auto OpenGLRenderer::draw_skybox() -> void {
    this->transform_uniform_buffer.set_data(OpenGLUniforms::Transform{
        .view_matrix = glm::mat4{glm::mat3{this->view_matrix}},
        .projection_matrix = this->projection_matrix
    });

    glCullFace(GL_FRONT);
    glDepthFunc(GL_LEQUAL);
    this->skybox_shader.bind();
    this->skybox.bind();
    this->cube.get_render_command()();
    this->skybox.unbind();
    glDepthFunc(GL_LESS);
    glCullFace(GL_BACK);
}

auto OpenGLRenderer::update_lights() -> void {
    const auto& light_data = this->get_light_manager().get_light_data();

    auto light_uniforms = OpenGLUniforms::Light{
        .directional_light =
            {.direction = {light_data.directional_light.direction},
             .ambient = {light_data.directional_light.ambient},
             .diffuse = {light_data.directional_light.diffuse},
             .specular = {light_data.directional_light.specular}},
        .point_light_count = light_data.point_light_count,
        .spot_light_count = light_data.spot_light_count
    };

    /// NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index)
    for (size_t i = 0; i < light_data.point_light_count; ++i) {
        light_uniforms.point_lights[i] = {
            .position = {light_data.point_lights[i].position},
            .ambient = {light_data.point_lights[i].ambient},
            .diffuse = {light_data.point_lights[i].diffuse},
            .specular = {light_data.point_lights[i].specular},
            .constant = light_data.point_lights[i].constant,
            .linear = light_data.point_lights[i].linear,
            .quadratic = light_data.point_lights[i].quadratic,
        };
    }

    for (size_t i = 0; i < light_data.spot_light_count; ++i) {
        light_uniforms.spot_lights[i] = {
            .position = {light_data.spot_lights[i].position},
            .direction = {light_data.spot_lights[i].direction},
            .ambient = {light_data.spot_lights[i].ambient},
            .diffuse = {light_data.spot_lights[i].diffuse},
            .specular = {light_data.spot_lights[i].specular},
            .constant = light_data.spot_lights[i].constant,
            .linear = light_data.spot_lights[i].linear,
            .quadratic = light_data.spot_lights[i].quadratic,
            .cut_off = light_data.spot_lights[i].cut_off,
            .outer_cut_off = light_data.spot_lights[i].outer_cut_off,
        };
    }
    /// NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index)

    this->light_uniform_buffer.set_data(light_uniforms);
}

auto OpenGLRenderer::get_framebuffer_resize_callback()
    -> Window::FramebufferSizeCallback {
    return [this](int width, int height) {
        glViewport(0, 0, width, height);
        this->hdr_frame_buffer.resize(width, height);
    };
}

}  // namespace Luminol::Graphics
