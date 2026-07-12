#include <LuminolRenderEngine/Utilities/ModelLoader.hpp>

#include <doctest/doctest.h>

using namespace Luminol::Utilities::ModelLoader;

TEST_CASE("load_model loads the cube fixture with sane mesh data") {
    const auto model = load_model("res/models/cube/cube.obj");

    REQUIRE(model.has_value());
    REQUIRE_FALSE(model->meshes.empty());

    const auto& mesh = model->meshes.front();

    REQUIRE_FALSE(mesh.indices.empty());
    CHECK(mesh.indices.size() % 3 == 0);
    CHECK_FALSE(mesh.vertices.empty());
    CHECK(mesh.vertices.size() == mesh.normals.size());
    CHECK(mesh.vertices.size() == mesh.texture_coordinates.size());
    CHECK(mesh.alpha_mode == AlphaMode::Opaque);
}

TEST_CASE("load_model returns nullopt for a nonexistent path") {
    const auto model = load_model("res/models/does_not_exist.obj");

    CHECK_FALSE(model.has_value());
}
