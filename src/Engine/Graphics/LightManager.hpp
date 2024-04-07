#pragma once

#include <set>
#include <map>
#include <optional>

#include <Engine/Graphics/Light.hpp>

namespace Luminol::Graphics {

class LightManager {
public:
    using PointLightId = uint32_t;

    LightManager();

    auto update_directional_light(const DirectionalLight& directional_light)
        -> void;

    [[nodiscard]] auto add_point_light(const PointLight& point_light)
        -> std::optional<PointLightId>;
    auto update_point_light(
        PointLightId point_light_id, const PointLight& point_light
    ) -> void;
    auto remove_point_light(PointLightId point_light_id) -> void;

    [[nodiscard]] auto get_light_data() -> const Light&;

private:
    Light light_data = {};

    std::set<PointLightId> free_point_light_ids;
    std::map<PointLightId, PointLight> point_lights_map;
};

}  // namespace Luminol::Graphics
