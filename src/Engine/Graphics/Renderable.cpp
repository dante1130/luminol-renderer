#include "Engine/Graphics/Renderable.hpp"

namespace Luminol::Graphics {

auto Renderable::get_material() const -> Material { return this->material; }

auto Renderable::set_material(const Material& material) -> void {
    this->material = material;
}

}  // namespace Luminol::Graphics
