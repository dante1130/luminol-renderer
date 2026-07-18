#include <cstdint>

#include <LuminolRenderEngine/Graphics/IdPool.hpp>

#include <doctest/doctest.h>

using namespace Luminol::Graphics;

TEST_CASE("unbounded pool hands out ids starting at 0") {
    auto pool = IdPool<uint32_t>{};

    CHECK(pool.allocate() == 0);
    CHECK(pool.allocate() == 1);
    CHECK(pool.allocate() == 2);
}

TEST_CASE("unbounded pool reuses a freed id before growing") {
    auto pool = IdPool<uint32_t>{};

    const auto id0 = pool.allocate();
    const auto id1 = pool.allocate();
    REQUIRE(id0 == 0);
    REQUIRE(id1 == 1);

    pool.free(*id0);

    CHECK(pool.allocate() == 0);
    CHECK(pool.allocate() == 2);
}

TEST_CASE("bounded pool returns nullopt once capacity is exhausted") {
    auto pool = IdPool<uint32_t>{2};

    CHECK(pool.allocate() == 0);
    CHECK(pool.allocate() == 1);
    CHECK(pool.allocate() == std::nullopt);
}

TEST_CASE("bounded pool can allocate again after a free") {
    auto pool = IdPool<uint32_t>{2};

    const auto id0 = pool.allocate();
    const auto id1 = pool.allocate();
    REQUIRE(id0 == 0);
    REQUIRE(id1 == 1);
    REQUIRE(pool.allocate() == std::nullopt);

    pool.free(*id0);

    CHECK(pool.allocate() == 0);
}

TEST_CASE("freeing a never-allocated id is a safe no-op") {
    auto pool = IdPool<uint32_t>{};

    pool.free(999);

    CHECK(pool.allocate() == 0);
}

TEST_CASE("multiple freed ids are reused lowest first") {
    auto pool = IdPool<uint32_t>{};

    const auto id0 = pool.allocate();
    const auto id1 = pool.allocate();
    const auto id2 = pool.allocate();
    REQUIRE(id0 == 0);
    REQUIRE(id1 == 1);
    REQUIRE(id2 == 2);

    pool.free(*id2);
    pool.free(*id0);

    CHECK(pool.allocate() == 0);
    CHECK(pool.allocate() == 2);
    CHECK(pool.allocate() == 3);
}
