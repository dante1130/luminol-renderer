#include "RenderableManager.hpp"

namespace Luminol::Graphics {

auto RenderableManager::allocate_id() -> RenderableId {
    return *this->id_pool.allocate();
}

auto RenderableManager::allocate_id(const std::filesystem::path& model_path)
    -> RenderableId {
    if (this->path_to_id.contains(model_path)) {
        return this->path_to_id.at(model_path);
    }

    const auto renderable_id = *this->id_pool.allocate();
    this->path_to_id[model_path] = renderable_id;
    this->id_to_path[renderable_id] = model_path;

    return renderable_id;
}

auto RenderableManager::remove_renderable(RenderableId renderable_id) -> void {
    this->id_pool.free(renderable_id);

    const auto found = this->id_to_path.find(renderable_id);
    if (found != this->id_to_path.end()) {
        this->path_to_id.erase(found->second);
        this->id_to_path.erase(found);
    }
}

}  // namespace Luminol::Graphics
