Texture2D albedo_texture : register(t0, space2);
Texture2D normal_texture : register(t1, space2);
Texture2D metallic_texture : register(t2, space2);
Texture2D roughness_texture : register(t3, space2);
Texture2D ao_texture : register(t4, space2);
Texture2D ssao_texture : register(t5, space2);
Texture2D shadow_map_texture : register(t6, space2);

SamplerState albedo_sampler : register(s0, space2);
SamplerState normal_sampler : register(s1, space2);
SamplerState metallic_sampler : register(s2, space2);
SamplerState roughness_sampler : register(s3, space2);
SamplerState ao_sampler : register(s4, space2);
SamplerState ssao_sampler : register(s5, space2);
SamplerComparisonState shadow_map_sampler : register(s6, space2);

TextureCube irradiance_texture : register(t7, space2);
TextureCube prefiltered_texture : register(t8, space2);
Texture2D brdf_lut_texture : register(t9, space2);

SamplerState irradiance_sampler : register(s7, space2);
SamplerState prefiltered_sampler : register(s8, space2);
SamplerState brdf_lut_sampler : register(s9, space2);

// Point/spot shadow maps for a capped, frame-selected subset of
// shadow-casting lights (see LightManager::update_shadow_casters,
// SDL_GPUPointSpotShadowPass). shadow_data.x / shadow_slot on
// PointLight/SpotLight (< 0 if unshadowed) index into these. Both are flat
// 2D atlases (one tile per light-face/light, see the ATLAS constants below)
// rather than texture arrays, so a single render pass can cover every
// shadow-casting light via viewport/scissor instead of one pass per layer.
// Must stay grouped with the texture/sampler resources above (not the
// StructuredBuffer block below) - SDL_GPU's fragment resource-space
// convention requires all textures/samplers declared contiguously before any
// storage buffers.
Texture2D point_shadow_maps : register(t10, space2);
Texture2D spot_shadow_maps : register(t11, space2);

SamplerComparisonState point_shadow_sampler : register(s10, space2);
SamplerComparisonState spot_shadow_sampler : register(s11, space2);

// Screen-space reflections for this pixel (rgb = reflected color, a = hit
// confidence), produced by SDL_GPUScreenSpaceReflectionPass. Must stay grouped
// with the textures/samplers above (before the storage buffers), per SDL_GPU's
// fragment resource-space convention. Kept identical to pbr_frag.hlsl so both
// fragment shaders share one resource layout.
Texture2D ssr_texture : register(t12, space2);
SamplerState ssr_sampler : register(s12, space2);

struct PointLight {
    float4 position;
    float4 color;
    // x: shadow map slot (< 0 if this light doesn't cast a shadow this
    // frame), else an index into point_shadow_maps. yzw: unused/reserved.
    float4 shadow_data;
};

struct SpotLight {
    float4 position;
    float4 direction;
    float3 color;
    float cut_off;
    float outer_cut_off;
    // Shadow map slot (< 0 if this light doesn't cast a shadow this frame),
    // else an index into spot_shadow_maps/spot_shadow_matrices.
    float shadow_slot;
    float2 spot_light_element_padding;
};

struct ClusterLightGrid {
    uint offset;
    uint point_count;
    uint spot_count;
    uint padding;
};

// Clustered Forward+ light buffers, populated by SDL_GPUClusterPass's
// count -> scan -> compact compute passes. Continue the t-register index
// right after the 12 textures/samplers above (t0-t11), per SDL_GPU's
// fragment resource-space convention (space2 = textures then storage
// buffers, in declaration order).
StructuredBuffer<PointLight> point_lights : register(t13, space2);
StructuredBuffer<SpotLight> spot_lights : register(t14, space2);
StructuredBuffer<ClusterLightGrid> cluster_light_grid : register(t15, space2);
StructuredBuffer<uint> global_light_index_list : register(t16, space2);
StructuredBuffer<row_major float4x4> spot_shadow_matrices : register(t17, space2);

// Must match SDL_GPUPointSpotShadowPass.cpp exactly.
static const float POINT_SHADOW_NEAR_PLANE = 0.05f;
static const float POINT_SHADOW_RESOLUTION = 512.0f;
static const float SPOT_SHADOW_RESOLUTION = 1024.0f;

// Atlas grid layout - must match point_atlas_cols/rows and
// spot_atlas_cols/rows in SDL_GPUPointSpotShadowPass.cpp exactly.
static const float POINT_ATLAS_COLS = 8.0f;
static const float POINT_ATLAS_ROWS = 12.0f;
static const float SPOT_ATLAS_COLS = 8.0f;
static const float SPOT_ATLAS_ROWS = 4.0f;

// Must match cluster_grid_x/y/z in SDL_GPUClusterPass.hpp.
static const uint CLUSTER_GRID_X = 16;
static const uint CLUSTER_GRID_Y = 9;
static const uint CLUSTER_GRID_Z = 24;

cbuffer LightBuffer : register(b0, space3) {
    float4 light_direction;
    float4 light_color;
    float4 view_position;
    float4 screen_size;
    row_major float4x4 light_space_matrix;
    // x: shadow map resolution, y: normal-offset bias,
    // z: max prefiltered specular mip level
    float4 shadow_params;
    // x: camera near plane, y: camera far plane, z/w: unused.
    float4 cluster_params;
};

// Must match the cluster index encoding in cluster_aabb_build.hlsl /
// cluster_light_count.hlsl / cluster_light_compact.hlsl exactly.
uint compute_cluster_index(
    float2 screen_position, float z_ndc, float near_plane, float far_plane
) {
    const float2 tile_uv = screen_position / screen_size.xy;

    const uint cluster_x =
        min(uint(tile_uv.x * float(CLUSTER_GRID_X)), CLUSTER_GRID_X - 1);
    // Viewport +Y is down, but cluster row 0 corresponds to NDC y=-1
    // (bottom, see cluster_aabb_build.hlsl), so flip.
    const uint cluster_y = min(
        uint((1.0f - tile_uv.y) * float(CLUSTER_GRID_Y)), CLUSTER_GRID_Y - 1
    );

    const float z_view = (near_plane * far_plane) /
        (far_plane - z_ndc * (far_plane - near_plane));
    const float slice_ratio = far_plane / near_plane;
    const uint cluster_z = min(
        uint(max(log(z_view / near_plane) / log(slice_ratio), 0.0f) *
             float(CLUSTER_GRID_Z)),
        CLUSTER_GRID_Z - 1
    );

    return cluster_z * (CLUSTER_GRID_X * CLUSTER_GRID_Y) +
        cluster_y * CLUSTER_GRID_X + cluster_x;
}

struct PSInput {
    float2 uv : TEXCOORD0;
    float3 world_position : TEXCOORD1;
    float3 world_normal : TEXCOORD2;
    float3 world_tangent : TEXCOORD3;
    float4 screen_position : SV_Position;
};

static const float PI = 3.14159265359f;

float distribution_ggx(float3 normal, float3 half_direction, float roughness) {
    const float a = roughness * roughness;
    const float numerator = a * a;

    const float n_dot_h = max(dot(normal, half_direction), 0.0f);
    const float n_dot_h_2 = n_dot_h * n_dot_h;

    float denominator = (n_dot_h_2 * (numerator - 1.0f) + 1.0f);
    denominator = PI * denominator * denominator;

    return numerator / denominator;
}

float visibility_smith_ggx_correlated(
    float n_dot_v, float n_dot_l, float roughness
) {
    const float a2 = roughness * roughness;
    const float ggx_v = n_dot_l * sqrt(n_dot_v * n_dot_v * (1.0f - a2) + a2);
    const float ggx_l = n_dot_v * sqrt(n_dot_l * n_dot_l * (1.0f - a2) + a2);

    return 0.5f / max(ggx_v + ggx_l, 0.0001f);
}

float3 fresnel_schlick(float cos_theta, float3 f0) {
    return f0 + (1.0f - f0) * pow(clamp(1.0f - cos_theta, 0.0f, 1.0f), 5.0f);
}

float3 fresnel_schlick_roughness(float cos_theta, float3 f0, float roughness) {
    const float3 one_minus_roughness =
        float3(1.0f - roughness, 1.0f - roughness, 1.0f - roughness);
    return f0 + (max(one_minus_roughness, f0) - f0)
        * pow(clamp(1.0f - cos_theta, 0.0f, 1.0f), 5.0f);
}

float specular_antialiasing(float3 normal, float roughness) {
    const float SPECULAR_AA_VARIANCE = 0.25f;
    const float SPECULAR_AA_THRESHOLD = 0.18f;

    const float3 dndu = ddx(normal);
    const float3 dndv = ddy(normal);
    const float variance = SPECULAR_AA_VARIANCE * (dot(dndu, dndu) + dot(dndv, dndv));

    const float kernel_roughness2 = min(2.0f * variance, SPECULAR_AA_THRESHOLD);
    const float filtered_roughness2 = saturate(roughness * roughness + kernel_roughness2);

    return sqrt(filtered_roughness2);
}

float2 env_brdf_approx(float n_dot_v, float roughness) {
    const float4 c0 = float4(-1.0f, -0.0275f, -0.572f, 0.022f);
    const float4 c1 = float4(1.0f, 0.0425f, 1.04f, -0.04f);
    const float4 r = roughness * c0 + c1;

    const float a004 = min(r.x * r.x, exp2(-9.28f * n_dot_v)) * r.x + r.y;

    return float2(-1.04f, 1.04f) * a004 + r.zw;
}

float3 energy_compensation(float3 f0, float n_dot_v, float roughness) {
    const float2 ab = env_brdf_approx(n_dot_v, roughness);
    const float ess = max(ab.x + ab.y, 0.001f);

    return 1.0f + f0 * (1.0f / ess - 1.0f);
}

float3 calculate_specular_brdf(
    float3 fresnel,
    float3 light_direction,
    float3 half_direction,
    float3 normal,
    float3 view_direction,
    float3 f0,
    float roughness
) {
    const float n_dot_v = max(dot(normal, view_direction), 0.0f);
    const float n_dot_l = max(dot(normal, light_direction), 0.0f);

    const float normal_distribution = distribution_ggx(normal, half_direction, roughness);
    const float visibility = visibility_smith_ggx_correlated(n_dot_v, n_dot_l, roughness);

    const float3 specular = normal_distribution * visibility * fresnel;

    return specular * energy_compensation(f0, n_dot_v, roughness);
}

float3 calculate_directional_light(
    float3 normal,
    float3 view_direction,
    float3 albedo,
    float3 f0,
    float metallic,
    float roughness
) {
    const float3 radiance = light_color.rgb;

    const float3 light_dir = normalize(-light_direction.xyz);
    const float3 half_direction = normalize(light_dir + view_direction);

    const float3 fresnel =
        fresnel_schlick(max(dot(half_direction, view_direction), 0.0f), f0);

    const float3 specular = calculate_specular_brdf(
        fresnel, light_dir, half_direction, normal, view_direction, f0, roughness
    );

    const float3 k_s = fresnel;
    float3 k_d = 1.0f - k_s;
    k_d *= 1.0f - metallic;

    const float n_dot_l = max(dot(normal, light_dir), 0.0f);

    return (k_d * albedo / PI + specular) * radiance * n_dot_l;
}

// Must match the cutoff in cluster_light_count.hlsl / cluster_light_compact.hlsl
// exactly, so a light's shaded falloff (via distance_window below) reaches
// zero at the same radius culling uses to exclude it entirely - otherwise
// lights would pop off abruptly at cluster boundaries instead of fading out.
float light_cull_radius(float3 color) {
    const float cutoff = 1.0f / 16.0f;
    const float intensity = max(color.r, max(color.g, color.b));
    return sqrt(max(intensity, 0.0f) / cutoff);
}

// Smoothly attenuates to zero at `radius` (Frostbite/UE4-style windowed
// falloff), keeping shading consistent with the hard radius used for
// culling.
float distance_window(float dist, float radius) {
    const float ratio = saturate(1.0f - pow(dist / radius, 4.0f));
    return ratio * ratio;
}

float3 calculate_point_light(
    PointLight light,
    float3 normal,
    float3 view_direction,
    float3 world_position,
    float3 albedo,
    float3 f0,
    float metallic,
    float roughness
) {
    const float distance = length(light.position.xyz - world_position);
    const float radius = light_cull_radius(light.color.rgb);
    const float attenuation =
        distance_window(distance, radius) / (distance * distance);

    const float3 radiance = light.color.rgb * attenuation;

    const float3 light_dir = normalize(light.position.xyz - world_position);
    const float3 half_direction = normalize(light_dir + view_direction);

    const float3 fresnel =
        fresnel_schlick(max(dot(half_direction, view_direction), 0.0f), f0);

    const float3 specular = calculate_specular_brdf(
        fresnel, light_dir, half_direction, normal, view_direction, f0, roughness
    );

    const float3 k_s = fresnel;
    float3 k_d = 1.0f - k_s;
    k_d *= 1.0f - metallic;

    const float n_dot_l = max(dot(normal, light_dir), 0.0f);

    return (k_d * albedo / PI + specular) * radiance * n_dot_l;
}

float3 calculate_spot_light(
    SpotLight light,
    float3 normal,
    float3 view_direction,
    float3 world_position,
    float3 albedo,
    float3 f0,
    float metallic,
    float roughness
) {
    const float3 light_dir = normalize(light.position.xyz - world_position);

    const float theta = dot(light_dir, normalize(-light.direction.xyz));
    const float epsilon = light.cut_off - light.outer_cut_off;
    const float intensity = saturate((theta - light.outer_cut_off) / epsilon);

    const float distance = length(light.position.xyz - world_position);
    const float radius = light_cull_radius(light.color);
    const float attenuation =
        distance_window(distance, radius) / (distance * distance);

    const float3 radiance = light.color * attenuation;

    const float3 half_direction = normalize(light_dir + view_direction);

    const float3 fresnel =
        fresnel_schlick(max(dot(half_direction, view_direction), 0.0f), f0);

    const float3 specular = calculate_specular_brdf(
        fresnel, light_dir, half_direction, normal, view_direction, f0, roughness
    );

    const float3 k_s = fresnel;
    float3 k_d = 1.0f - k_s;
    k_d *= 1.0f - metallic;

    const float n_dot_l = max(dot(normal, light_dir), 0.0f);

    return (k_d * albedo / PI + specular) * radiance * n_dot_l * intensity;
}

float calculate_shadow(float3 world_position, float3 normal) {
    const float normal_offset_bias = shadow_params.y;
    const float3 offset_position = world_position + normal * normal_offset_bias;

    const float4 light_space_position =
        mul(float4(offset_position, 1.0f), light_space_matrix);

    float2 shadow_uv = light_space_position.xy * 0.5f + 0.5f;
    shadow_uv.y = 1.0f - shadow_uv.y;
    const float fragment_depth = light_space_position.z;

    if (fragment_depth < 0.0f || fragment_depth > 1.0f ||
        any(shadow_uv < 0.0f) || any(shadow_uv > 1.0f)) {
        return 1.0f;
    }

    const float texel_size = 1.0f / max(shadow_params.x, 1.0f);
    const float constant_bias = 0.0015f;

    float visibility = 0.0f;
    [unroll]
    for (int x = -1; x <= 1; ++x) {
        [unroll]
        for (int y = -1; y <= 1; ++y) {
            const float2 offset = float2(x, y) * texel_size;
            visibility += shadow_map_texture.SampleCmpLevelZero(
                shadow_map_sampler, shadow_uv + offset, fragment_depth - constant_bias
            );
        }
    }

    return visibility / 9.0f;
}

// Cube face basis vectors (right, up, forward), derived from
// SDL_GPUPointSpotShadowPass.cpp's cube_faces target_offset/up pairs run
// through the same left_handed_look_at_matrix formula used to render each
// face (right = normalize(cross(up_input, forward)), up = cross(forward,
// right)) - hardcoded here since both inputs are compile-time axis-aligned
// constants. Order matches cube_faces: +X,-X,+Y,-Y,+Z,-Z.
static const float3 POINT_FACE_RIGHT[6] = {
    float3(0.0f, 0.0f, -1.0f),
    float3(0.0f, 0.0f, 1.0f),
    float3(1.0f, 0.0f, 0.0f),
    float3(1.0f, 0.0f, 0.0f),
    float3(1.0f, 0.0f, 0.0f),
    float3(-1.0f, 0.0f, 0.0f),
};
static const float3 POINT_FACE_UP[6] = {
    float3(0.0f, 1.0f, 0.0f),
    float3(0.0f, 1.0f, 0.0f),
    float3(0.0f, 0.0f, -1.0f),
    float3(0.0f, 0.0f, 1.0f),
    float3(0.0f, 1.0f, 0.0f),
    float3(0.0f, 1.0f, 0.0f),
};
static const float3 POINT_FACE_FORWARD[6] = {
    float3(1.0f, 0.0f, 0.0f),
    float3(-1.0f, 0.0f, 0.0f),
    float3(0.0f, 1.0f, 0.0f),
    float3(0.0f, -1.0f, 0.0f),
    float3(0.0f, 0.0f, 1.0f),
    float3(0.0f, 0.0f, -1.0f),
};

// Point lights are a flat 2D atlas (see the ATLAS constants above), so there
// is no hardware cube-face selection or cross-face filtering. This manually
// picks the dominant-axis face (matching cube_faces' +X,-X,+Y,-Y,+Z,-Z
// order), projects the direction into that face's local UV using the same
// view/projection convention SDL_GPUPointSpotShadowPass used to render it,
// then maps (slot, face) to its atlas tile. The compare depth re-derivation
// is unchanged - it only depends on the dominant axis's magnitude, not which
// face it belongs to.
float calculate_point_shadow(float3 world_position, float3 normal, PointLight light) {
    const int shadow_slot = (int)round(light.shadow_data.x);
    if (shadow_slot < 0) {
        return 1.0f;
    }

    const float normal_offset_bias = shadow_params.y;
    const float3 offset_position = world_position + normal * normal_offset_bias;
    const float3 light_to_fragment = offset_position - light.position.xyz;

    const float3 abs_direction = abs(light_to_fragment);

    int face;
    if (abs_direction.x >= abs_direction.y && abs_direction.x >= abs_direction.z) {
        face = light_to_fragment.x > 0.0f ? 0 : 1;
    } else if (abs_direction.y >= abs_direction.x && abs_direction.y >= abs_direction.z) {
        face = light_to_fragment.y > 0.0f ? 2 : 3;
    } else {
        face = light_to_fragment.z > 0.0f ? 4 : 5;
    }

    const float3 view_direction = float3(
        dot(light_to_fragment, POINT_FACE_RIGHT[face]),
        dot(light_to_fragment, POINT_FACE_UP[face]),
        dot(light_to_fragment, POINT_FACE_FORWARD[face])
    );
    const float major_axis = view_direction.z;

    const float near_plane = POINT_SHADOW_NEAR_PLANE;
    const float far_plane = light_cull_radius(light.color.rgb);
    const float range = far_plane - near_plane;
    const float ndc_depth =
        (far_plane / range) - ((near_plane * far_plane) / (range * major_axis));

    float2 local_uv = (view_direction.xy / major_axis) * 0.5f + 0.5f;
    local_uv.y = 1.0f - local_uv.y;

    // Inset away from the tile edges so the 3x3 PCF kernel below never reads
    // into a neighboring tile's data (no hardware cross-face clamping with a
    // flat atlas).
    const float tile_inset = 1.5f / POINT_SHADOW_RESOLUTION;
    local_uv = clamp(local_uv, tile_inset, 1.0f - tile_inset);

    const float tile_index = float(shadow_slot) * 6.0f + float(face);
    const float2 tile_coord =
        float2(fmod(tile_index, POINT_ATLAS_COLS), floor(tile_index / POINT_ATLAS_COLS));
    const float2 atlas_uv =
        (tile_coord + local_uv) / float2(POINT_ATLAS_COLS, POINT_ATLAS_ROWS);

    const float2 texel_size = float2(
        1.0f / (POINT_ATLAS_COLS * POINT_SHADOW_RESOLUTION),
        1.0f / (POINT_ATLAS_ROWS * POINT_SHADOW_RESOLUTION)
    );
    const float constant_bias = 0.0015f;

    float visibility = 0.0f;
    [unroll]
    for (int x = -1; x <= 1; ++x) {
        [unroll]
        for (int y = -1; y <= 1; ++y) {
            const float2 offset = float2(x, y) * texel_size;
            visibility += point_shadow_maps.SampleCmpLevelZero(
                point_shadow_sampler, atlas_uv + offset, ndc_depth - constant_bias
            );
        }
    }

    return visibility / 9.0f;
}

float calculate_spot_shadow(float3 world_position, float3 normal, SpotLight light) {
    const int shadow_slot = (int)round(light.shadow_slot);
    if (shadow_slot < 0) {
        return 1.0f;
    }

    const float normal_offset_bias = shadow_params.y;
    const float3 offset_position = world_position + normal * normal_offset_bias;

    const float4x4 light_space_matrix_for_slot = spot_shadow_matrices[shadow_slot];
    const float4 light_space_position =
        mul(float4(offset_position, 1.0f), light_space_matrix_for_slot);
    const float inv_w = 1.0f / light_space_position.w;

    float2 local_uv = (light_space_position.xy * inv_w) * 0.5f + 0.5f;
    local_uv.y = 1.0f - local_uv.y;
    const float fragment_depth = light_space_position.z * inv_w;

    if (fragment_depth < 0.0f || fragment_depth > 1.0f ||
        any(local_uv < 0.0f) || any(local_uv > 1.0f)) {
        return 1.0f;
    }

    // Inset away from the tile edges so the 3x3 PCF kernel below never reads
    // into a neighboring tile's data (no per-layer isolation with a flat
    // atlas).
    const float tile_inset = 1.5f / SPOT_SHADOW_RESOLUTION;
    local_uv = clamp(local_uv, tile_inset, 1.0f - tile_inset);

    const float2 tile_coord = float2(
        fmod(float(shadow_slot), SPOT_ATLAS_COLS),
        floor(float(shadow_slot) / SPOT_ATLAS_COLS)
    );
    const float2 atlas_uv =
        (tile_coord + local_uv) / float2(SPOT_ATLAS_COLS, SPOT_ATLAS_ROWS);

    const float2 texel_size = float2(
        1.0f / (SPOT_ATLAS_COLS * SPOT_SHADOW_RESOLUTION),
        1.0f / (SPOT_ATLAS_ROWS * SPOT_SHADOW_RESOLUTION)
    );
    const float constant_bias = 0.0015f;

    float visibility = 0.0f;
    [unroll]
    for (int x = -1; x <= 1; ++x) {
        [unroll]
        for (int y = -1; y <= 1; ++y) {
            const float2 offset = float2(x, y) * texel_size;
            visibility += spot_shadow_maps.SampleCmpLevelZero(
                spot_shadow_sampler, atlas_uv + offset, fragment_depth - constant_bias
            );
        }
    }

    return visibility / 9.0f;
}

float4 main(PSInput input) : SV_Target {
    const float4 albedo_alpha = albedo_texture.Sample(albedo_sampler, input.uv);

    // glTF MASK alpha mode: discard fragments below the cutoff instead of
    // blending. All alpha-tested materials seen so far use the glTF-default
    // cutoff of 0.5.
    clip(albedo_alpha.a - 0.5f);

    const float3 albedo = albedo_alpha.rgb;

    float3 normal_map = normal_texture.Sample(normal_sampler, input.uv).rgb;
    normal_map = normalize(normal_map * 2.0f - 1.0f);

    const float3 N = normalize(input.world_normal);
    const float3 T = normalize(input.world_tangent - N * dot(input.world_tangent, N));
    const float3 B = cross(N, T);
    const float3x3 tbn = float3x3(T, B, N);

    const float3 normal = normalize(mul(normal_map, tbn));

    const float metallic = metallic_texture.Sample(metallic_sampler, input.uv).b;
    const float roughness =
        specular_antialiasing(normal, roughness_texture.Sample(roughness_sampler, input.uv).g);
    const float ao = ao_texture.Sample(ao_sampler, input.uv).r;

    const float2 screen_uv = input.screen_position.xy / screen_size.xy;
    const float ssao = ssao_texture.Sample(ssao_sampler, screen_uv).r;

    const float3 view_direction = normalize(view_position.xyz - input.world_position);

    float3 f0 = float3(0.04f, 0.04f, 0.04f);
    f0 = lerp(f0, albedo, metallic);

    const float3 directional_lo = calculate_directional_light(
        normal, view_direction, albedo, f0, metallic, roughness
    );

    const uint cluster_index = compute_cluster_index(
        input.screen_position.xy, input.screen_position.z,
        cluster_params.x, cluster_params.y
    );
    const ClusterLightGrid grid = cluster_light_grid[cluster_index];

    float3 point_lo = float3(0.0f, 0.0f, 0.0f);
    [loop]
    for (uint i = 0; i < grid.point_count; ++i) {
        const uint light_index = global_light_index_list[grid.offset + i];
        const PointLight point_light = point_lights[light_index];
        const float point_shadow =
            calculate_point_shadow(input.world_position, N, point_light);
        point_lo += calculate_point_light(
            point_light, normal, view_direction, input.world_position,
            albedo, f0, metallic, roughness
        ) * point_shadow;
    }

    float3 spot_lo = float3(0.0f, 0.0f, 0.0f);
    [loop]
    for (uint i = 0; i < grid.spot_count; ++i) {
        const uint light_index =
            global_light_index_list[grid.offset + grid.point_count + i];
        const SpotLight spot_light = spot_lights[light_index];
        const float spot_shadow =
            calculate_spot_shadow(input.world_position, N, spot_light);
        spot_lo += calculate_spot_light(
            spot_light, normal, view_direction, input.world_position,
            albedo, f0, metallic, roughness
        ) * spot_shadow;
    }

    const float shadow = calculate_shadow(input.world_position, N);

    const float3 R = reflect(-view_direction, normal);
    const float n_dot_v = max(dot(normal, view_direction), 0.0f);

    const float3 k_s = fresnel_schlick_roughness(n_dot_v, f0, roughness);
    const float3 k_d = (1.0f - k_s) * (1.0f - metallic);

    const float3 irradiance = irradiance_texture.Sample(irradiance_sampler, normal).rgb;
    const float3 diffuse_ibl = k_d * irradiance * albedo;

    const float max_reflection_lod = shadow_params.z;
    const float3 prefiltered_color = prefiltered_texture.SampleLevel(
        prefiltered_sampler, R, roughness * max_reflection_lod
    ).rgb;
    const float2 env_brdf = brdf_lut_texture.Sample(brdf_lut_sampler, float2(n_dot_v, roughness)).rg;

    // Screen-space reflections replace the global prefiltered reflection where
    // the SSR pass found a confident on-screen hit, faded out with roughness.
    // Matches pbr_frag.hlsl. screen_uv is computed earlier in main().
    const float4 ssr = ssr_texture.Sample(ssr_sampler, screen_uv);
    const float ssr_weight = ssr.a * (1.0f - roughness);
    const float3 reflection_color = lerp(prefiltered_color, ssr.rgb, ssr_weight);
    const float3 specular_ibl = reflection_color * (k_s * env_brdf.x + env_brdf.y);

    const float3 ambient = (diffuse_ibl + specular_ibl) * ao * ssao;
    const float3 color = ambient + directional_lo * shadow + point_lo + spot_lo;

    return float4(color, albedo_alpha.a);
}
