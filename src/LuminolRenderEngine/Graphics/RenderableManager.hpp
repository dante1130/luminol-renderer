#pragma once

#include <cstdint>
#include <filesystem>
#include <unordered_map>

#include <LuminolRenderEngine/Graphics/IdPool.hpp>

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
    IdPool<RenderableId> id_pool;
    std::unordered_map<std::filesystem::path, RenderableId> path_to_id;
    std::unordered_map<RenderableId, std::filesystem::path> id_to_path;
};

}  // namespace Luminol::Graphics
