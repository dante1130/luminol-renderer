#include <Engine/LuminolEngine.hpp>

auto main() -> int {
    auto luminol_engine = Luminol::Engine(Luminol::Properties{});
    luminol_engine.run();

    return 0;
}
