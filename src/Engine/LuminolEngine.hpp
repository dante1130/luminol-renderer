#pragma once

#include <cstdint>
#include <string>
#include <memory>

#include <Engine/Window/Window.hpp>
#include <Engine/Graphics/Renderer.hpp>
#include <Engine/Graphics/GraphicsFactory.hpp>

namespace Luminol {

constexpr static auto default_width = 640;
constexpr static auto default_height = 480;

struct Properties {
    int32_t width = {default_width};
    int32_t height = {default_height};
    std::string title = "Luminol Engine";
};

class Engine {
public:
    Engine(const Properties& properties);
    void run();

private:
    Window window;
    std::unique_ptr<Graphics::GraphicsFactory> graphics_factory = nullptr;
    std::unique_ptr<Graphics::Renderer> renderer = nullptr;
};

}  // namespace Luminol