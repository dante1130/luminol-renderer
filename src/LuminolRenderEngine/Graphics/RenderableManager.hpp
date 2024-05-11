#pragma once

#include <cstdint>
#include <set>
#include <unordered_map>
#include <filesystem>

#include <gsl/gsl>

#include <LuminolRenderEngine/Graphics/GraphicsApi.hpp>
#include <LuminolRenderEngine/Graphics/Renderable.hpp>
#include <LuminolRenderEngine/Graphics/TexturePaths.hpp>

namespace Luminol::Graphics {

class GraphicsFactory;

class RenderableManager {
public:
    RenderableManager(GraphicsApi api);

    [[nodiscard]] auto get_graphics_factory() const
        -> const Graphics::GraphicsFactory&;

    [[nodiscard]] auto create_renderable(
        gsl::span<const float> vertices,
        gsl::span<const uint32_t> indices,
        const TexturePaths& texture_paths
    ) -> RenderableId;

    [[nodiscard]] auto create_renderable(const std::filesystem::path& model_path
    ) -> RenderableId;

    [[nodiscard]] auto get_renderable(RenderableId renderable_id) const
        -> const Renderable&;

    auto remove_renderable(RenderableId renderable_id) -> void;

private:
    [[nodiscard]] auto get_free_renderable_id() -> RenderableId;

    std::shared_ptr<Graphics::GraphicsFactory> graphics_factory = nullptr;

    std::set<RenderableId> used_renderable_ids;
    std::unordered_map<std::filesystem::path, RenderableId> renderable_ids_map;
    std::unordered_map<RenderableId, std::unique_ptr<Renderable>>
        renderables_map;
};

}  // namespace Luminol::Graphics
