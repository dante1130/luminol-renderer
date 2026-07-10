#pragma once

#include <set>
#include <map>
#include <optional>

#include <LuminolMaths/Matrix.hpp>
#include <LuminolMaths/Vector.hpp>

#include <LuminolRenderEngine/Graphics/Light.hpp>

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

    // Re-scores every active point/spot light against the camera's frustum
    // (in-frustum, nearest/brightest first) and updates which lights hold a
    // shadow slot, with hysteresis so lights near the cap boundary don't
    // flicker in/out of shadowing every frame. Must be called before
    // get_light_data() so the repacked arrays reflect this frame's
    // assignment.
    auto update_shadow_casters(
        const Maths::Matrix4x4f& view_projection_matrix,
        const Maths::Vector3f& camera_position
    ) -> void;

    [[nodiscard]] auto get_light_data() -> const Light&;

private:
    Light light_data = {};

    std::set<LightId> free_point_light_ids;
    std::set<LightId> free_spot_light_ids;
    std::map<LightId, PointLight> point_lights_map;
    std::map<LightId, SpotLight> spot_lights_map;

    std::map<LightId, uint32_t> point_shadow_slots;
    std::map<LightId, uint32_t> spot_shadow_slots;
};

}  // namespace Luminol::Graphics
