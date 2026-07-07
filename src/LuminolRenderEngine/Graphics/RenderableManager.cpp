#include "RenderableManager.hpp"

namespace Luminol::Graphics {

auto RenderableManager::allocate_id() -> RenderableId {
    return this->get_free_renderable_id();
}

auto RenderableManager::allocate_id(const std::filesystem::path& model_path)
    -> RenderableId {
    if (this->renderable_ids_map.contains(model_path)) {
        return this->renderable_ids_map.at(model_path);
    }

    const auto renderable_id = this->get_free_renderable_id();
    this->renderable_ids_map[model_path] = renderable_id;

    return renderable_id;
}

auto RenderableManager::remove_renderable(RenderableId renderable_id) -> void {
    this->used_renderable_ids.erase(renderable_id);

    for (const auto& [model_path, id] : this->renderable_ids_map) {
        if (id == renderable_id) {
            this->renderable_ids_map.erase(model_path);
            break;
        }
    }
}

auto RenderableManager::get_free_renderable_id() -> RenderableId {
    if (this->used_renderable_ids.empty()) {
        this->used_renderable_ids.emplace(0);
        return 0;
    }

    const auto free_renderable_id = *this->used_renderable_ids.crbegin() + 1;
    this->used_renderable_ids.emplace(free_renderable_id);

    return free_renderable_id;
}

}  // namespace Luminol::Graphics
