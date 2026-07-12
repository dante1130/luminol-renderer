// Screen-space reflections.
//
// Runs as a fullscreen pass (fullscreen_vert.hlsl) after the AO normal
// prepass has written this frame's view-space normals + depth. It traces a
// mirror reflection ray in view space against the depth buffer and, on a hit,
// samples the PREVIOUS frame's resolved HDR color as the reflected radiance.
// The output is (rgb = reflected color, a = confidence); the forward PBR pass
// blends this over the global prefiltered-cubemap specular IBL weighted by
// that confidence, so a miss (or an invalid previous frame) falls back cleanly
// to the existing IBL.

Texture2D depth_texture : register(t0, space2);
Texture2D normal_texture : register(t1, space2);
Texture2D previous_color_texture : register(t2, space2);

SamplerState depth_sampler : register(s0, space2);
SamplerState normal_sampler : register(s1, space2);
SamplerState color_sampler : register(s2, space2);

// params:  x = max_distance (view-space units the ray may travel),
//          y = thickness (view-space depth tolerance for a hit),
//          z = max_steps (upper bound on the screen-space march samples),
//          w = valid_previous (0 or 1).
// viewport_size: xy = width, height.
cbuffer SSRBuffer : register(b0, space3) {
    row_major float4x4 projection_matrix;
    row_major float4x4 inverse_projection_matrix;
    float4 params;
    float4 viewport_size;
};

struct PSInput {
    float2 uv : TEXCOORD0;
};

// Matches ssao_frag.hlsl: NDC from uv (note the left-handed projection uses
// depth in [0,1]) reprojected to view space via the inverse projection.
float3 reconstruct_view_position(float2 uv, float depth) {
    const float2 ndc = uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f);
    float4 view_position = mul(float4(ndc, depth, 1.0f), inverse_projection_matrix);
    view_position /= view_position.w;
    return view_position.xyz;
}

// Cheap per-pixel pseudo-random value in [0,1), used to jitter the ray start
// so the residual stair-stepping becomes fine noise instead of coherent
// aliased edges (same technique as ssao_frag.hlsl).
float interleaved_gradient_noise(float2 pixel_coord) {
    const float3 magic = float3(0.06711056f, 0.00583715f, 52.9829189f);
    return frac(magic.z * frac(dot(pixel_coord, magic.xy)));
}

float4 main(PSInput input) : SV_Target {
    const float max_distance = params.x;
    const float thickness = params.y;
    const int max_steps = (int)params.z;
    const float valid_previous = params.w;

    const float depth = depth_texture.Sample(depth_sampler, input.uv).r;
    if (depth >= 1.0f || valid_previous < 0.5f) {
        return float4(0.0f, 0.0f, 0.0f, 0.0f);
    }

    const float3 view_position = reconstruct_view_position(input.uv, depth);
    const float3 view_normal =
        normalize(normal_texture.Sample(normal_sampler, input.uv).rgb * 2.0f - 1.0f);

    // View-space camera is at the origin, so the incident direction (camera ->
    // surface) is just the normalized surface position.
    const float3 incident = normalize(view_position);
    const float3 reflect_dir = normalize(reflect(incident, view_normal));

    // Reflection heading back toward the camera plane (negative view z) can't
    // be traced forward against the depth buffer - reject it.
    if (reflect_dir.z <= 0.0f) {
        return float4(0.0f, 0.0f, 0.0f, 0.0f);
    }

    // March in SCREEN space rather than in uniform view-space steps: at
    // grazing angles a fixed view-space step covers many pixels, undersampling
    // the reflected line and producing aliased edges. Projecting the ray's
    // endpoints and walking ~one pixel per step keeps the sampling uniform on
    // screen regardless of angle. Depth is interpolated perspective-correctly
    // via z/w and 1/w (both linear in screen space).
    const float3 ray_start = view_position;
    const float3 ray_end = view_position + reflect_dir * max_distance;

    const float4 clip_start = mul(float4(ray_start, 1.0f), projection_matrix);
    const float4 clip_end = mul(float4(ray_end, 1.0f), projection_matrix);

    const float inv_w_start = 1.0f / clip_start.w;
    const float inv_w_end = 1.0f / clip_end.w;

    const float2 uv_start =
        clip_start.xy * inv_w_start * float2(0.5f, -0.5f) + 0.5f;
    const float2 uv_end = clip_end.xy * inv_w_end * float2(0.5f, -0.5f) + 0.5f;

    // z/w at each endpoint - linearly interpolable across the screen segment.
    const float z_over_w_start = ray_start.z * inv_w_start;
    const float z_over_w_end = ray_end.z * inv_w_end;

    const float2 pixel_delta = (uv_end - uv_start) * viewport_size.xy;
    const float pixel_length = max(abs(pixel_delta.x), abs(pixel_delta.y));
    const int steps = clamp((int)pixel_length, 1, max_steps);

    const float jitter =
        interleaved_gradient_noise(input.uv * viewport_size.xy);

    // Perspective-correct view-space z of the ray at parameter t in [0,1].
    float previous_t = 0.0f;

    [loop]
    for (int i = 1; i <= steps; ++i) {
        const float t = (i - jitter) / steps;
        const float2 sample_uv = lerp(uv_start, uv_end, t);

        // Ray left the screen - no on-screen geometry to reflect.
        if (sample_uv.x < 0.0f || sample_uv.x > 1.0f ||
            sample_uv.y < 0.0f || sample_uv.y > 1.0f) {
            break;
        }

        const float inv_w = lerp(inv_w_start, inv_w_end, t);
        const float ray_z = lerp(z_over_w_start, z_over_w_end, t) / inv_w;

        const float scene_depth = depth_texture.Sample(depth_sampler, sample_uv).r;
        if (scene_depth >= 1.0f) {
            previous_t = t;
            continue;
        }

        const float scene_z =
            reconstruct_view_position(sample_uv, scene_depth).z;

        // Positive when the ray sample has passed BEHIND the scene surface
        // (larger view z = further from camera in this left-handed setup).
        const float delta = ray_z - scene_z;

        if (delta > 0.0f) {
            // Crossed the surface between the previous and current sample.
            // Ignore crossings deeper than the thickness band - those are
            // rays passing behind unrelated foreground geometry.
            if (delta < thickness) {
                // Binary-search refine the crossing (in screen-space t) for a
                // sharper hit.
                float lo = previous_t;
                float hi = t;
                [unroll]
                for (int r = 0; r < 5; ++r) {
                    const float mid = (lo + hi) * 0.5f;
                    const float2 mid_uv = lerp(uv_start, uv_end, mid);
                    const float mid_inv_w = lerp(inv_w_start, inv_w_end, mid);
                    const float mid_ray_z =
                        lerp(z_over_w_start, z_over_w_end, mid) / mid_inv_w;
                    const float mid_scene_depth =
                        depth_texture.Sample(depth_sampler, mid_uv).r;
                    const float mid_scene_z =
                        reconstruct_view_position(mid_uv, mid_scene_depth).z;
                    if (mid_ray_z - mid_scene_z > 0.0f) {
                        hi = mid;
                    } else {
                        lo = mid;
                    }
                }

                const float2 hit_uv = lerp(uv_start, uv_end, hi);
                const float3 hit_color =
                    previous_color_texture.Sample(color_sampler, hit_uv).rgb;

                // Fade out as the hit approaches the screen edges (off-screen
                // data is unavailable) and as the ray length grows (less
                // reliable far hits).
                const float2 edge = smoothstep(0.0f, 0.15f, hit_uv) *
                    smoothstep(0.0f, 0.15f, 1.0f - hit_uv);
                const float edge_fade = edge.x * edge.y;
                const float distance_fade = saturate(1.0f - hi);

                return float4(hit_color, edge_fade * distance_fade);
            }
            break;
        }

        previous_t = t;
    }

    return float4(0.0f, 0.0f, 0.0f, 0.0f);
}
