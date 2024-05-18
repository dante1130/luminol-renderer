#include "OpenGLAutoExposureRenderPass.hpp"

#include <glad/gl.h>

namespace {

using namespace Luminol::Graphics;

auto create_compute_shader() -> OpenGLShader {
    auto compute_shader = OpenGLShader{
        ShaderPaths{.compute_shader_path = "res/shaders/test_compute.glsl"}
    };

    compute_shader.bind();
    compute_shader.set_image_binding_point(
        "screen_image", ImageBindingPoint::Screen
    );
    compute_shader.unbind();

    return compute_shader;
}

}  // namespace

namespace Luminol::Graphics {

OpenGLAutoExposureRenderPass::OpenGLAutoExposureRenderPass(
    int32_t width, int32_t height
)
    : compute_shader{create_compute_shader()},
      screen_texture{width, height, TextureInternalFormat::RGBA32F} {}

auto OpenGLAutoExposureRenderPass::draw(int32_t width, int32_t height) const
    -> void {
    constexpr auto work_group_size = 16;

    this->compute_shader.bind();
    this->screen_texture.bind_image(
        ImageBindingPoint::Screen, ImageAccess::Write
    );
    this->compute_shader.dispatch_compute(
        width / work_group_size, height / work_group_size, 1
    );
    this->compute_shader.unbind();
}

}  // namespace Luminol::Graphics
