#pragma once

#include <array>
#include <cstdint>
#include <limits>
#include <optional>

#include <LuminolMaths/Matrix.hpp>
#include <LuminolMaths/Vector.hpp>

#include <LuminolRenderEngine/Graphics/IdPool.hpp>
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

    // Sentinel stored in point_shadow_slots/spot_shadow_slots for a light
    // that doesn't currently hold a shadow slot.
    static constexpr auto no_shadow_slot = std::numeric_limits<uint32_t>::max();

private:
    Light light_data = {};

    IdPool<LightId> point_light_ids;
    IdPool<LightId> spot_light_ids;

    // Dense, fixed-capacity storage indexed directly by LightId (ids are
    // handed out from a bounded 0..max_point_lights-1 / 0..max_spot_lights-1
    // pool - see point_light_ids/spot_light_ids), so every per-frame
    // lookup/update is a direct array index instead of a tree traversal.
    // Active flags use
    // uint8_t rather than array<bool, N> so they're span-able (array<bool, N>
    // has no special packing like vector<bool>, but uint8_t keeps this
    // consistent with the rest of the flags here).
    std::array<PointLight, max_point_lights> point_lights{};
    std::array<std::uint8_t, max_point_lights> point_light_active{};
    std::array<uint32_t, max_point_lights> point_shadow_slots{};

    std::array<SpotLight, max_spot_lights> spot_lights{};
    std::array<std::uint8_t, max_spot_lights> spot_light_active{};
    std::array<uint32_t, max_spot_lights> spot_shadow_slots{};
};

}  // namespace Luminol::Graphics
