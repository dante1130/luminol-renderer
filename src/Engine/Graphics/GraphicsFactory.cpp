#include "GraphicsFactory.hpp"

#include <Engine/Graphics/OpenGL/OpenGLFactory.hpp>

namespace Luminol::Graphics {

auto GraphicsFactory::create(GraphicsApi graphics_api)
    -> std::unique_ptr<GraphicsFactory> {
    switch (graphics_api) {
        case GraphicsApi::OpenGL:
            return std::make_unique<OpenGLFactory>();
        default:
            throw std::runtime_error("Unsupported graphics API");
    }
}

}  // namespace Luminol::Graphics
