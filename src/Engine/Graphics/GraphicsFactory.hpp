#pragma once

#include <memory>

#include <Engine/Graphics/GraphicsApi.hpp>
#include <Engine/Graphics/Renderer.hpp>
#include <Engine/Window/Window.hpp>

namespace Luminol::Graphics {

class GraphicsFactory {
public:
    GraphicsFactory() = default;
    virtual ~GraphicsFactory() = default;
    GraphicsFactory(const GraphicsFactory&) = default;
    GraphicsFactory(GraphicsFactory&&) = delete;
    auto operator=(const GraphicsFactory&) -> GraphicsFactory& = default;
    auto operator=(GraphicsFactory&&) -> GraphicsFactory& = delete;

    [[nodiscard]] virtual auto create_renderer(const Window& window)
        -> std::unique_ptr<Renderer> = 0;
    [[nodiscard]] virtual auto get_graphics_api() const -> GraphicsApi = 0;
};

}  // namespace Luminol::Graphics
