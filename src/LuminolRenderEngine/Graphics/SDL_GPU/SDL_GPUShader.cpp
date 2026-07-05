#include "SDL_GPUShader.hpp"

namespace Luminol::Graphics::SDL_GPU {

Shader::Shader(std::unique_ptr<SDL_GPUShader, SDL_GPUShaderDeleter> shader)
    : shader{std::move(shader)} {
    Expects(this->shader);
}

auto Shader::get() const -> SDL_GPUShader* { return shader.get(); }

}  // namespace Luminol::Graphics::SDL_GPU
