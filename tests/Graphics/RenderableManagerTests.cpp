#include <filesystem>

#include <LuminolRenderEngine/Graphics/RenderableManager.hpp>

#include <doctest/doctest.h>

using namespace Luminol::Graphics;

TEST_CASE("first allocate_id returns 0") {
    auto manager = RenderableManager{};

    CHECK(manager.allocate_id() == 0);
}

TEST_CASE("allocate_id(path) returns the same id for the same path") {
    auto manager = RenderableManager{};

    const auto path = std::filesystem::path{"res/models/cube/cube.obj"};
    const auto first_id = manager.allocate_id(path);
    const auto second_id = manager.allocate_id(path);

    CHECK(first_id == second_id);
}

TEST_CASE("allocate_id(path) returns different ids for different paths") {
    auto manager = RenderableManager{};

    const auto id_a = manager.allocate_id(std::filesystem::path{"a.obj"});
    const auto id_b = manager.allocate_id(std::filesystem::path{"b.obj"});

    CHECK(id_a != id_b);
}

TEST_CASE("removing a non-max id leaves a permanent gap, not reused") {
    auto manager = RenderableManager{};

    const auto id0 = manager.allocate_id();
    const auto id1 = manager.allocate_id();
    const auto id2 = manager.allocate_id();
    REQUIRE(id0 == 0);
    REQUIRE(id1 == 1);
    REQUIRE(id2 == 2);

    manager.remove_renderable(id1);

    const auto next_id = manager.allocate_id();
    CHECK(next_id == 3);
}

TEST_CASE("removing the current max id makes that slot reusable") {
    auto manager = RenderableManager{};

    const auto id0 = manager.allocate_id();
    const auto id1 = manager.allocate_id();
    const auto id2 = manager.allocate_id();
    REQUIRE(id0 == 0);
    REQUIRE(id1 == 1);
    REQUIRE(id2 == 2);

    manager.remove_renderable(id2);

    const auto next_id = manager.allocate_id();
    CHECK(next_id == 2);
}

TEST_CASE("removing a never-allocated id is a safe no-op") {
    auto manager = RenderableManager{};

    manager.remove_renderable(999);

    CHECK(manager.allocate_id() == 0);
}

TEST_CASE("plain and path-based allocate_id share the same id space") {
    auto manager = RenderableManager{};

    const auto id0 = manager.allocate_id();
    const auto id1 = manager.allocate_id(std::filesystem::path{"a.obj"});
    const auto id2 = manager.allocate_id();

    CHECK(id0 == 0);
    CHECK(id1 == 1);
    CHECK(id2 == 2);
}
