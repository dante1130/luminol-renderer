#pragma once

#include <vector>

#include <LuminolMaths/Vector.hpp>

namespace Luminol::Graphics {

struct DirectionalLight {
    Maths::Vector3f direction = {0.5f, -0.5f, 1.0f};
    Maths::Vector3f color = {1.0f, 1.0f, 1.0f};
};

// Raised from 64 now that lighting is Clustered Forward+ (per-cluster light
// index lists via SDL_GPUClusterPass) rather than a fixed-size cbuffer array
// in pbr_frag.hlsl, so the cap no longer needs to fit inside a uniform
// buffer.
constexpr static auto max_point_lights = 1024u;

struct PointLight {
    Maths::Vector3f position = {0.0f, 0.0f, 0.0f};
    Maths::Vector3f color = {1.0f, 1.0f, 1.0f};
};

constexpr static auto max_spot_lights = 1024u;
constexpr static auto default_cut_off = 0.0f;
constexpr static auto default_outer_cut_off = 0.0f;

struct SpotLight {
    Maths::Vector3f position = {0.0f, 0.0f, 0.0f};
    Maths::Vector3f direction = {0.0f, 0.0f, 0.0f};
    Maths::Vector3f color = {1.0f, 1.0f, 1.0f};
    float cut_off = 0.0f;
    float outer_cut_off = 0.0f;
};

struct AlignedDirectionalLight {
    Maths::Vector4f direction =
        Maths::Vector4f{0.5f, -0.5f, 1.0f, 0.0f};  // 16 bytes
    Maths::Vector4f color =
        Maths::Vector4f{1.0f, 1.0f, 1.0f, 1.0f};  // 16 bytes
};

// Point/spot lights are far too numerous (up to max_point_lights /
// max_spot_lights) to each get a full shadow map, so only the
// highest-scoring (nearest/brightest, in-frustum) lights cast shadows each
// frame. See LightManager::update_shadow_casters (selection/hysteresis) and
// SDL_GPUPointSpotShadowPass (rendering into the shadow slots these produce).
constexpr static auto max_shadow_casting_point_lights = 16u;
constexpr static auto max_shadow_casting_spot_lights = 32u;

struct AlignedPointLight {
    Maths::Vector4f position =
        Maths::Vector4f{0.0f, 0.0f, 0.0f, 0.0f};  // 16 bytes
    Maths::Vector4f color =
        Maths::Vector4f{1.0f, 1.0f, 1.0f, 1.0f};  // 16 bytes
    // x: shadow map slot (-1 if this light doesn't cast a shadow this frame,
    // else an index into SDL_GPUPointSpotShadowPass's point shadow cube
    // array), yzw: unused/reserved.
    Maths::Vector4f shadow_data =
        Maths::Vector4f{-1.0f, 0.0f, 0.0f, 0.0f};  // 16 bytes
};

struct AlignedSpotLight {
    Maths::Vector4f position =
        Maths::Vector4f{0.0f, 0.0f, 0.0f, 0.0f};  // 16 bytes
    Maths::Vector4f direction =
        Maths::Vector4f{0.0f, 0.0f, 0.0f, 0.0f};                // 16 bytes
    Maths::Vector3f color = Maths::Vector3f{1.0f, 1.0f, 1.0f};  // 12 bytes
    float cut_off = default_cut_off;                            // 4 bytes
    float outer_cut_off = default_outer_cut_off;                // 4 bytes
    // Shadow map slot (-1 if this light doesn't cast a shadow this frame,
    // else an index into SDL_GPUPointSpotShadowPass's spot shadow 2D array
    // and shadow-matrix buffer).
    float shadow_slot = -1.0f;                                  // 4 bytes
    Maths::Vector2f padding = Maths::Vector2f{0.0f, 0.0f};      // 8 bytes
};

struct Light {
    AlignedDirectionalLight directional_light;              // 64 bytes
    uint32_t point_light_count = 0;                         // 4 bytes
    uint32_t spot_light_count = 0;                          // 4 bytes
    Maths::Vector2f padding = Maths::Vector2f{0.0f, 0.0f};  // 8 bytes
    std::vector<AlignedPointLight> point_lights =
        std::vector<AlignedPointLight>{max_point_lights};
    std::vector<AlignedSpotLight> spot_lights =
        std::vector<AlignedSpotLight>{max_spot_lights};
};

constexpr static auto alignment = 16u;

static_assert(sizeof(AlignedDirectionalLight) % alignment == 0);
static_assert(sizeof(AlignedPointLight) % alignment == 0);
static_assert(sizeof(AlignedSpotLight) % alignment == 0);
static_assert(sizeof(Light) % alignment == 0);

}  // namespace Luminol::Graphics
