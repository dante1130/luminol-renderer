#pragma once

#include <set>
#include <map>
#include <optional>

#include <Engine/Graphics/Light.hpp>

namespace Luminol::Graphics {

class LightManager {
public:
    using LightId = uint32_t;

    LightManager();

    auto update_directional_light(const DirectionalLight& directional_light)
        -> void;

    [[nodiscard]] auto add_point_light(const PointLight& point_light)
        -> std::optional<LightId>;
    auto update_point_light(
        LightId point_light_id, const PointLight& point_light
    ) -> void;
    auto remove_point_light(LightId point_light_id) -> void;

    [[nodiscard]] auto add_spot_light(const SpotLight& spot_light)
        -> std::optional<LightId>;
    auto update_spot_light(LightId spot_light_id, const SpotLight& spot_light)
        -> void;
    auto remove_spot_light(LightId spot_light_id) -> void;

    [[nodiscard]] auto get_light_data() -> const Light&;

private:
    Light light_data = {};

    std::set<LightId> free_point_light_ids;
    std::set<LightId> free_spot_light_ids;
    std::map<LightId, PointLight> point_lights_map;
    std::map<LightId, SpotLight> spot_lights_map;
};

}  // namespace Luminol::Graphics
