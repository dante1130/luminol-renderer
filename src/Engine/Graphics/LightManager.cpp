#include "LightManager.hpp"

#include <gsl/gsl>

namespace {

using namespace Luminol::Graphics;

auto create_free_light_ids(uint32_t max_lights)
    -> std::set<LightManager::LightId> {
    auto free_light_ids = std::set<LightManager::LightId>{};

    for (auto i = 0u; i < max_lights; ++i) {
        free_light_ids.insert(i);
    }

    return free_light_ids;
}

}  // namespace

namespace Luminol::Graphics {

LightManager::LightManager()
    : free_point_light_ids{create_free_light_ids(max_point_lights)},
      free_spot_light_ids{create_free_light_ids(max_spot_lights)} {}

auto LightManager::update_directional_light(
    const DirectionalLight& directional_light
) -> void {
    this->light_data.directional_light.direction = directional_light.direction;
    this->light_data.directional_light.ambient = directional_light.ambient;
    this->light_data.directional_light.diffuse = directional_light.diffuse;
    this->light_data.directional_light.specular = directional_light.specular;
}

auto LightManager::add_point_light(const PointLight& point_light)
    -> std::optional<LightId> {
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
    LightId point_light_id, const PointLight& point_light
) -> void {
    if (!this->point_lights_map.contains(point_light_id)) {
        return;
    }

    this->point_lights_map[point_light_id] = point_light;
}

auto LightManager::remove_point_light(LightId point_light_id) -> void {
    if (!this->point_lights_map.contains(point_light_id)) {
        return;
    }

    this->free_point_light_ids.insert(point_light_id);
    this->point_lights_map.erase(point_light_id);
    this->light_data.point_light_count =
        gsl::narrow<uint32_t>(this->point_lights_map.size());
}

auto LightManager::add_spot_light(const SpotLight& spot_light)
    -> std::optional<LightId> {
    if (this->free_spot_light_ids.empty()) {
        return std::nullopt;
    }

    const auto spot_light_id = *this->free_spot_light_ids.begin();
    this->free_spot_light_ids.erase(spot_light_id);

    this->spot_lights_map[spot_light_id] = spot_light;
    this->light_data.spot_light_count =
        gsl::narrow<uint32_t>(this->spot_lights_map.size());

    return spot_light_id;
}

auto LightManager::update_spot_light(
    LightId spot_light_id, const SpotLight& spot_light
) -> void {
    if (!this->spot_lights_map.contains(spot_light_id)) {
        return;
    }

    this->spot_lights_map[spot_light_id] = spot_light;
}

auto LightManager::remove_spot_light(LightId spot_light_id) -> void {
    if (!this->spot_lights_map.contains(spot_light_id)) {
        return;
    }

    this->free_spot_light_ids.insert(spot_light_id);
    this->spot_lights_map.erase(spot_light_id);
    this->light_data.spot_light_count =
        gsl::narrow<uint32_t>(this->spot_lights_map.size());
}

[[nodiscard]] auto LightManager::get_light_data() -> const Light& {
    /// NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index)
    {
        size_t array_index = 0;
        for (const auto& [point_light_id, point_light] :
             this->point_lights_map) {
            this->light_data.point_lights[array_index] = point_light;
            ++array_index;
        }
    }

    {
        size_t array_index = 0;
        for (const auto& [spot_light_id, spot_light] : this->spot_lights_map) {
            this->light_data.spot_lights[array_index] = spot_light;
            ++array_index;
        }
    }
    /// NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index)

    return this->light_data;
}

}  // namespace Luminol::Graphics