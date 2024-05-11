#include "RenderableManager.hpp"

namespace Luminol::Graphics {

RenderableManager::RenderableManager(GraphicsApi api)
    : graphics_factory(GraphicsFactory::create(api)) {}

auto RenderableManager::get_graphics_factory() const -> const GraphicsFactory& {
    return *this->graphics_factory;
}

auto RenderableManager::create_renderable(
    gsl::span<const float> vertices,
    gsl::span<const uint32_t> indices,
    const TexturePaths& texture_paths
) -> RenderableId {
    auto renderable =
        this->graphics_factory->create_mesh(vertices, indices, texture_paths);

    auto renderable_id = this->get_free_renderable_id();

    this->renderables_map[renderable_id] = std::move(renderable);

    return renderable_id;
}

auto RenderableManager::create_renderable(
    const std::filesystem::path& model_path
) -> RenderableId {
    if (this->renderable_ids_map.contains(model_path)) {
        return this->renderable_ids_map.at(model_path);
    }

    auto renderable = this->graphics_factory->create_model(model_path);

    auto renderable_id = this->get_free_renderable_id();

    this->renderable_ids_map[model_path] = renderable_id;
    this->renderables_map[renderable_id] = std::move(renderable);

    return renderable_id;
}

auto RenderableManager::get_renderable(RenderableId renderable_id) const
    -> const Renderable& {
    return *this->renderables_map.at(renderable_id);
}

auto RenderableManager::remove_renderable(RenderableId renderable_id) -> void {
    this->renderables_map.erase(renderable_id);
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

    auto free_renderable_id = *this->used_renderable_ids.crbegin() + 1;
    this->used_renderable_ids.emplace(free_renderable_id);

    return free_renderable_id;
}

}  // namespace Luminol::Graphics
