#pragma once

#include <cstdint>
#include <set>
#include <unordered_map>
#include <filesystem>

namespace Luminol::Graphics {

using RenderableId = uint32_t;
using FontId = uint32_t;

class RenderableManager {
public:
    [[nodiscard]] auto allocate_id() -> RenderableId;
    [[nodiscard]] auto allocate_id(const std::filesystem::path& model_path)
        -> RenderableId;

    auto remove_renderable(RenderableId renderable_id) -> void;

private:
    [[nodiscard]] auto get_free_renderable_id() -> RenderableId;

    std::set<RenderableId> used_renderable_ids;
    std::unordered_map<std::filesystem::path, RenderableId> renderable_ids_map;
};

}  // namespace Luminol::Graphics
