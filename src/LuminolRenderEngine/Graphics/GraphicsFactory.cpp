#include "GraphicsFactory.hpp"

#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLFactory.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUFactory.hpp>

namespace Luminol::Graphics {

auto GraphicsFactory::create(GraphicsApi graphics_api, uint32_t msaa_sample_count)
    -> std::shared_ptr<GraphicsFactory> {
    switch (graphics_api) {
        case GraphicsApi::OpenGL:
            return std::make_shared<OpenGLFactory>();
        case GraphicsApi::SDL_GPU:
            return std::make_shared<SDL_GPU::SDL_GPUFactory>(msaa_sample_count);
        default:
            throw std::runtime_error("Unsupported graphics API");
    }
}

}  // namespace Luminol::Graphics
