#include "LightManager.hpp"

#include <gsl/gsl>

namespace {

using namespace Luminol::Graphics;

auto create_free_point_light_ids() -> std::set<LightManager::PointLightId> {
    auto free_point_light_ids = std::set<LightManager::PointLightId>{};

    for (auto i = 0u; i < max_point_lights; ++i) {
        free_point_light_ids.insert(i);
    }

    return free_point_light_ids;
}

}  // namespace

namespace Luminol::Graphics {

LightManager::LightManager()
    : free_point_light_ids{create_free_point_light_ids()} {}

auto LightManager::update_directional_light(
    const DirectionalLight& directional_light
) -> void {
    this->light_data.directional_light.direction = directional_light.direction;
    this->light_data.directional_light.ambient = directional_light.ambient;
    this->light_data.directional_light.diffuse = directional_light.diffuse;
    this->light_data.directional_light.specular = directional_light.specular;
}

auto LightManager::add_point_light(const PointLight& point_light)
    -> std::optional<PointLightId> {
    if (this->free_point_light_ids.empty()) {
        return std::nullopt;
    }

    const auto point_light_id = *this->free_point_light_ids.begin();
    this->free_point_light_ids.erase(point_light_id);

    this->point_lights_map[point_light_id] = point_light;
    this->light_data.point_light_count =
        gsl::narrow<uint32_t>(this->point_lights_map.size());

    return point_light_id;
}

auto LightManager::update_point_light(
    PointLightId point_light_id, const PointLight& point_light
) -> void {
    if (!this->point_lights_map.contains(point_light_id)) {
        return;
    }

    this->point_lights_map[point_light_id] = point_light;
}

auto LightManager::remove_point_light(PointLightId point_light_id) -> void {
    if (!this->point_lights_map.contains(point_light_id)) {
        return;
    }

    this->free_point_light_ids.insert(point_light_id);
    this->point_lights_map.erase(point_light_id);
    this->light_data.point_light_count =
        gsl::narrow<uint32_t>(this->point_lights_map.size());
}

[[nodiscard]] auto LightManager::get_light_data() -> const Light& {
    size_t array_index = 0;
    /// NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index)
    for (const auto& [point_light_id, point_light] : this->point_lights_map) {
        this->light_data.point_lights[array_index] = point_light;
        ++array_index;
    }
    /// NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index)

    return this->light_data;
}

}  // namespace Luminol::Graphics
