#pragma once

#include <set>
#include <optional>
#include <unordered_map>

#include <Engine/Graphics/Light.hpp>

namespace Luminol::Graphics {

class LightManager {
public:
    using PointLightId = uint32_t;

    LightManager();

    auto update_directional_light(const DirectionalLight& directional_light)
        -> void;

    auto add_point_light(const PointLight& point_light)
        -> std::optional<PointLightId>;
    auto update_point_light(
        PointLightId point_light_id, const PointLight& point_light
    ) -> void;
    auto remove_point_light(PointLightId point_light_id) -> void;

    [[nodiscard]] auto get_light_data() const -> const Light&;

private:
    Light light_data = {};

    std::set<PointLightId> free_point_light_ids;
    std::unordered_map<PointLightId, PointLight> point_lights_map;
};

}  // namespace Luminol::Graphics
