#include "LightManager.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

#include <gsl/gsl>

namespace {

using namespace Luminol::Graphics;
using namespace Luminol::Maths;

auto create_free_light_ids(uint32_t max_lights)
    -> std::set<LightManager::LightId> {
    auto free_light_ids = std::set<LightManager::LightId>{};

    for (auto i = 0u; i < max_lights; ++i) {
        free_light_ids.insert(i);
    }

    return free_light_ids;
}

// Must match the light_cull_radius cutoff in pbr_frag.hlsl /
// cluster_light_count.hlsl / cluster_light_compact.hlsl exactly, so a
// shadow-caster's frustum test is consistent with the sphere used to cull
// its shading contribution to zero.
auto light_cull_radius(const Vector3f& color) -> float {
    constexpr auto cutoff = 1.0f / 16.0f;
    const auto intensity = std::max({color.x(), color.y(), color.z()});
    return std::sqrt(std::max(intensity, 0.0f) / cutoff);
}

// Gribb/Hartmann frustum plane extraction for a row-major view-projection
// matrix used with row-vector multiplication (mul(pos, vp), see
// pbr_vert.hlsl), so each plane's coefficients come from a column of the
// matrix rather than a row.
auto extract_frustum_planes(const Matrix4x4f& view_projection)
    -> std::array<Vector4f, 6> {
    const auto column = [&view_projection](int index) {
        return Vector4f{
            view_projection[0][index],
            view_projection[1][index],
            view_projection[2][index],
            view_projection[3][index],
        };
    };

    const auto col0 = column(0);
    const auto col1 = column(1);
    const auto col2 = column(2);
    const auto col3 = column(3);

    return std::array<Vector4f, 6>{
        col3 + col0,  // left
        col3 - col0,  // right
        col3 + col1,  // bottom
        col3 - col1,  // top
        col2,         // near
        col3 - col2,  // far
    };
}

auto sphere_in_frustum(
    const std::array<Vector4f, 6>& planes,
    const Vector3f& center,
    float radius
) -> bool {
    for (const auto& plane : planes) {
        const auto normal_length = std::sqrt(
            (plane.x() * plane.x()) + (plane.y() * plane.y()) +
            (plane.z() * plane.z())
        );
        if (normal_length < 1e-6f) {
            continue;
        }

        const auto distance =
            ((plane.x() * center.x()) + (plane.y() * center.y()) +
             (plane.z() * center.z()) + plane.w()) /
            normal_length;

        if (distance < -radius) {
            return false;
        }
    }

    return true;
}

struct ShadowCandidate {
    LightManager::LightId id;
    float score;
};

// Re-ranks lights_map by an in-frustum, nearest/brightest-first score, then
// updates current_slots so at most max_slots lights hold a slot. A light
// already holding a slot keeps it as long as it's still within a soft margin
// of the cutoff rank (not just the strict top max_slots), which stops lights
// that hover right at the boundary from popping their shadow in and out
// every frame; it can never displace a strictly higher-ranked light though,
// so the physical slot count is always respected.
template <typename LightT>
auto update_shadow_slot_assignments(
    const std::map<LightManager::LightId, LightT>& lights_map,
    std::map<LightManager::LightId, uint32_t>& current_slots,
    const std::array<Vector4f, 6>& frustum_planes,
    const Vector3f& camera_position,
    uint32_t max_slots
) -> void {
    auto candidates = std::vector<ShadowCandidate>{};
    candidates.reserve(lights_map.size());

    for (const auto& [id, light] : lights_map) {
        const auto radius = light_cull_radius(light.color);
        if (!sphere_in_frustum(frustum_planes, light.position, radius)) {
            continue;
        }

        const auto to_light = light.position - camera_position;
        const auto distance_squared = to_light.dot(to_light);
        const auto intensity =
            std::max({light.color.x(), light.color.y(), light.color.z()});
        const auto score = intensity / std::max(distance_squared, 0.01f);

        candidates.push_back(ShadowCandidate{.id = id, .score = score});
    }

    std::ranges::sort(candidates, [](const auto& lhs, const auto& rhs) {
        return lhs.score > rhs.score;
    });

    const auto strict_count =
        std::min(candidates.size(), static_cast<size_t>(max_slots));
    const auto soft_limit = static_cast<size_t>(
        std::ceil(static_cast<float>(max_slots) * 1.25f)
    );
    const auto soft_count = std::min(candidates.size(), soft_limit);

    auto new_slots = std::map<LightManager::LightId, uint32_t>{};
    auto used_slots = std::vector<bool>(max_slots, false);

    // Strict winners: always get a slot, preferring to keep whichever slot
    // they already held.
    auto unassigned_winners = std::vector<LightManager::LightId>{};
    for (size_t i = 0; i < strict_count; ++i) {
        const auto id = candidates[i].id;
        const auto existing = current_slots.find(id);
        if (existing != current_slots.end() &&
            existing->second < max_slots && !used_slots[existing->second]) {
            new_slots[id] = existing->second;
            used_slots[existing->second] = true;
        } else {
            unassigned_winners.push_back(id);
        }
    }

    auto next_free_slot = [&used_slots, max_slots]() -> std::optional<uint32_t> {
        for (auto slot = 0u; slot < max_slots; ++slot) {
            if (!used_slots[slot]) {
                return slot;
            }
        }
        return std::nullopt;
    };

    for (const auto id : unassigned_winners) {
        const auto slot = next_free_slot();
        if (!slot.has_value()) {
            break;
        }
        new_slots[id] = *slot;
        used_slots[*slot] = true;
    }

    // Boundary lights (ranked between the strict cap and the soft margin):
    // keep their previous slot if it's still free, but never take a slot
    // away from a strict winner.
    for (size_t i = strict_count; i < soft_count; ++i) {
        const auto id = candidates[i].id;
        const auto existing = current_slots.find(id);
        if (existing != current_slots.end() &&
            existing->second < max_slots && !used_slots[existing->second]) {
            new_slots[id] = existing->second;
            used_slots[existing->second] = true;
        }
    }

    current_slots = std::move(new_slots);
}

}  // namespace

namespace Luminol::Graphics {

LightManager::LightManager()
    : free_point_light_ids{create_free_light_ids(max_point_lights)},
      free_spot_light_ids{create_free_light_ids(max_spot_lights)} {}

auto LightManager::update_directional_light(
    const DirectionalLight& directional_light
) -> void {
    this->light_data.directional_light = AlignedDirectionalLight{
        .direction =
            Maths::Vector4f{
                directional_light.direction.x(),
                directional_light.direction.y(),
                directional_light.direction.z(),
                0.0f,
            },
        .color =
            Maths::Vector4f{
                directional_light.color.x(),
                directional_light.color.y(),
                directional_light.color.z(),
                1.0f,
            },
    };
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

auto LightManager::update_shadow_casters(
    const Maths::Matrix4x4f& view_projection_matrix,
    const Maths::Vector3f& camera_position
) -> void {
    const auto frustum_planes = extract_frustum_planes(view_projection_matrix);

    update_shadow_slot_assignments(
        this->point_lights_map, this->point_shadow_slots, frustum_planes,
        camera_position, max_shadow_casting_point_lights
    );
    update_shadow_slot_assignments(
        this->spot_lights_map, this->spot_shadow_slots, frustum_planes,
        camera_position, max_shadow_casting_spot_lights
    );
}

[[nodiscard]] auto LightManager::get_light_data() -> const Light& {
    /// NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index)
    {
        size_t array_index = 0;
        for (const auto& [point_light_id, point_light] :
             this->point_lights_map) {
            const auto shadow_slot_it =
                this->point_shadow_slots.find(point_light_id);
            const auto shadow_slot = shadow_slot_it != this->point_shadow_slots.end()
                ? static_cast<float>(shadow_slot_it->second)
                : -1.0f;

            this->light_data.point_lights[array_index] = AlignedPointLight{
                .position =
                    Maths::Vector4f{
                        point_light.position.x(),
                        point_light.position.y(),
                        point_light.position.z(),
                        1.0f,
                    },
                .color =
                    Maths::Vector4f{
                        point_light.color.x(),
                        point_light.color.y(),
                        point_light.color.z(),
                        1.0f,
                    },
                .shadow_data = Maths::Vector4f{shadow_slot, 0.0f, 0.0f, 0.0f},
            };

            ++array_index;
        }
    }

    {
        size_t array_index = 0;
        for (const auto& [spot_light_id, spot_light] : this->spot_lights_map) {
            const auto shadow_slot_it =
                this->spot_shadow_slots.find(spot_light_id);
            const auto shadow_slot = shadow_slot_it != this->spot_shadow_slots.end()
                ? static_cast<float>(shadow_slot_it->second)
                : -1.0f;

            this->light_data.spot_lights[array_index] = AlignedSpotLight{
                .position =
                    Maths::Vector4f{
                        spot_light.position.x(),
                        spot_light.position.y(),
                        spot_light.position.z(),
                        1.0f,
                    },
                .direction =
                    Maths::Vector4f{
                        spot_light.direction.x(),
                        spot_light.direction.y(),
                        spot_light.direction.z(),
                        0.0f,
                    },
                .color =
                    Maths::Vector3f{
                        spot_light.color.x(),
                        spot_light.color.y(),
                        spot_light.color.z(),
                    },
                .cut_off = spot_light.cut_off,
                .outer_cut_off = spot_light.outer_cut_off,
                .shadow_slot = shadow_slot,
            };

            ++array_index;
        }
    }
    /// NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index)

    return this->light_data;
}

}  // namespace Luminol::Graphics
