#include "Renderer.hpp"

namespace Luminol::Graphics {

auto Renderer::get_light_manager() -> LightManager& {
    return this->light_manager;
}

}  // namespace Luminol::Graphics
