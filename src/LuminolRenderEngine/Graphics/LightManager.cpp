#include "LightManager.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <span>
#include <vector>

#include <gsl/gsl>

#include <LuminolRenderEngine/Graphics/Frustum.hpp>

namespace {

using namespace Luminol::Graphics;
using namespace Luminol::Maths;

// Must match the light_cull_radius cutoff in pbr_frag.hlsl /
// cluster_light_count.hlsl / cluster_light_compact.hlsl exactly, so a
// shadow-caster's frustum test is consistent with the sphere used to cull
// its shading contribution to zero.
auto light_cull_radius(const Vector3f& color) -> float {
    constexpr auto cutoff = 1.0f / 16.0f;
    const auto intensity = std::max({color.x(), color.y(), color.z()});
    return std::sqrt(std::max(intensity, 0.0f) / cutoff);
}

using Luminol::Graphics::extract_frustum_planes;
using Luminol::Graphics::sphere_in_frustum;

struct ShadowCandidate {
    LightManager::LightId id;
    float score;
};

// Re-ranks the active lights in `lights` by an in-frustum,
// nearest/brightest-first score, then updates current_slots so at most
// max_slots lights hold a slot. A light already holding a slot keeps it as
// long as it's still within a soft margin of the cutoff rank (not just the
// strict top max_slots), which stops lights that hover right at the boundary
// from popping their shadow in and out every frame; it can never displace a
// strictly higher-ranked light though, so the physical slot count is always
// respected.
template <typename LightT>
auto update_shadow_slot_assignments(
    std::span<const LightT> lights,
    std::span<const std::uint8_t> active,
    std::span<uint32_t> current_slots,
    const std::array<Vector4f, 6>& frustum_planes,
    const Vector3f& camera_position,
    uint32_t max_slots
) -> void {
    auto candidates = std::vector<ShadowCandidate>{};
    candidates.reserve(lights.size());

    for (auto id = 0u; id < lights.size(); ++id) {
        if (active[id] == 0) {
            continue;
        }

        const auto& light = lights[id];
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

    auto new_slots = std::vector<uint32_t>(
        current_slots.size(), LightManager::no_shadow_slot
    );
    auto used_slots = std::vector<bool>(max_slots, false);

    // Strict winners: always get a slot, preferring to keep whichever slot
    // they already held.
    auto unassigned_winners = std::vector<LightManager::LightId>{};
    for (size_t i = 0; i < strict_count; ++i) {
        const auto id = candidates[i].id;
        const auto existing = current_slots[id];
        if (existing != LightManager::no_shadow_slot &&
            existing < max_slots && !used_slots[existing]) {
            new_slots[id] = existing;
            used_slots[existing] = true;
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
        const auto existing = current_slots[id];
        if (existing != LightManager::no_shadow_slot &&
            existing < max_slots && !used_slots[existing]) {
            new_slots[id] = existing;
            used_slots[existing] = true;
        }
    }

    std::ranges::copy(new_slots, current_slots.begin());
}

}  // namespace

namespace Luminol::Graphics {

LightManager::LightManager()
    : point_light_ids{max_point_lights}, spot_light_ids{max_spot_lights} {
    point_shadow_slots.fill(LightManager::no_shadow_slot);
    spot_shadow_slots.fill(LightManager::no_shadow_slot);
}

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
    const auto point_light_id = this->point_light_ids.allocate();
    if (!point_light_id.has_value()) {
        return std::nullopt;
    }

    gsl::at(this->point_lights, *point_light_id) = point_light;
    gsl::at(this->point_light_active, *point_light_id) = 1;
    ++this->light_data.point_light_count;

    return point_light_id;
}

auto LightManager::update_point_light(
    LightId point_light_id, const PointLight& point_light
) -> void {
    if (gsl::at(this->point_light_active, point_light_id) == 0) {
        return;
    }

    gsl::at(this->point_lights, point_light_id) = point_light;
}

auto LightManager::remove_point_light(LightId point_light_id) -> void {
    if (gsl::at(this->point_light_active, point_light_id) == 0) {
        return;
    }

    this->point_light_ids.free(point_light_id);
    gsl::at(this->point_light_active, point_light_id) = 0;
    gsl::at(this->point_shadow_slots, point_light_id) =
        LightManager::no_shadow_slot;
    --this->light_data.point_light_count;
}

auto LightManager::add_spot_light(const SpotLight& spot_light)
    -> std::optional<LightId> {
    const auto spot_light_id = this->spot_light_ids.allocate();
    if (!spot_light_id.has_value()) {
        return std::nullopt;
    }

    gsl::at(this->spot_lights, *spot_light_id) = spot_light;
    gsl::at(this->spot_light_active, *spot_light_id) = 1;
    ++this->light_data.spot_light_count;

    return spot_light_id;
}

auto LightManager::update_spot_light(
    LightId spot_light_id, const SpotLight& spot_light
) -> void {
    if (gsl::at(this->spot_light_active, spot_light_id) == 0) {
        return;
    }

    gsl::at(this->spot_lights, spot_light_id) = spot_light;
}

auto LightManager::remove_spot_light(LightId spot_light_id) -> void {
    if (gsl::at(this->spot_light_active, spot_light_id) == 0) {
        return;
    }

    this->spot_light_ids.free(spot_light_id);
    gsl::at(this->spot_light_active, spot_light_id) = 0;
    gsl::at(this->spot_shadow_slots, spot_light_id) =
        LightManager::no_shadow_slot;
    --this->light_data.spot_light_count;
}

auto LightManager::update_shadow_casters(
    const Maths::Matrix4x4f& view_projection_matrix,
    const Maths::Vector3f& camera_position
) -> void {
    const auto frustum_planes = extract_frustum_planes(view_projection_matrix);

    update_shadow_slot_assignments<PointLight>(
        this->point_lights, this->point_light_active, this->point_shadow_slots,
        frustum_planes, camera_position, max_shadow_casting_point_lights
    );
    update_shadow_slot_assignments<SpotLight>(
        this->spot_lights, this->spot_light_active, this->spot_shadow_slots,
        frustum_planes, camera_position, max_shadow_casting_spot_lights
    );
}

[[nodiscard]] auto LightManager::get_light_data() -> const Light& {
    /// NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index)
    {
        size_t array_index = 0;
        for (auto point_light_id = 0u; point_light_id < this->point_lights.size();
             ++point_light_id) {
            if (this->point_light_active[point_light_id] == 0) {
                continue;
            }

            const auto& point_light = this->point_lights[point_light_id];
            const auto shadow_slot_value =
                this->point_shadow_slots[point_light_id];
            const auto shadow_slot = shadow_slot_value != LightManager::no_shadow_slot
                ? static_cast<float>(shadow_slot_value)
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
        for (auto spot_light_id = 0u; spot_light_id < this->spot_lights.size();
             ++spot_light_id) {
            if (this->spot_light_active[spot_light_id] == 0) {
                continue;
            }

            const auto& spot_light = this->spot_lights[spot_light_id];
            const auto shadow_slot_value =
                this->spot_shadow_slots[spot_light_id];
            const auto shadow_slot = shadow_slot_value != LightManager::no_shadow_slot
                ? static_cast<float>(shadow_slot_value)
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
