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
//          z = step_count (linear march samples), w = valid_previous (0 or 1).
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

float2 view_to_uv(float3 view_position) {
    float4 clip = mul(float4(view_position, 1.0f), projection_matrix);
    clip.xyz /= clip.w;
    return clip.xy * float2(0.5f, -0.5f) + 0.5f;
}

float4 main(PSInput input) : SV_Target {
    const float max_distance = params.x;
    const float thickness = params.y;
    const int step_count = (int)params.z;
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

    const float step_length = max_distance / max(1, step_count);

    float3 previous_sample_pos = view_position;

    [loop]
    for (int i = 1; i <= step_count; ++i) {
        const float3 sample_pos = view_position + reflect_dir * (step_length * i);
        const float2 sample_uv = view_to_uv(sample_pos);

        // Ray left the screen - no on-screen geometry to reflect.
        if (sample_uv.x < 0.0f || sample_uv.x > 1.0f ||
            sample_uv.y < 0.0f || sample_uv.y > 1.0f) {
            break;
        }

        const float scene_depth = depth_texture.Sample(depth_sampler, sample_uv).r;
        if (scene_depth >= 1.0f) {
            previous_sample_pos = sample_pos;
            continue;
        }

        const float3 scene_view_pos =
            reconstruct_view_position(sample_uv, scene_depth);

        // Positive when the ray sample has passed BEHIND the scene surface
        // (larger view z = further from camera in this left-handed setup).
        const float delta = sample_pos.z - scene_view_pos.z;

        if (delta > 0.0f) {
            // Crossed the surface between the previous and current sample.
            // Ignore crossings deeper than the thickness band - those are
            // rays passing behind unrelated foreground geometry.
            if (delta < thickness) {
                // Binary-search refine the crossing for a sharper hit.
                float3 lo = previous_sample_pos;
                float3 hi = sample_pos;
                [unroll]
                for (int r = 0; r < 5; ++r) {
                    const float3 mid = (lo + hi) * 0.5f;
                    const float2 mid_uv = view_to_uv(mid);
                    const float mid_scene_depth =
                        depth_texture.Sample(depth_sampler, mid_uv).r;
                    const float3 mid_scene_pos =
                        reconstruct_view_position(mid_uv, mid_scene_depth);
                    if (mid.z - mid_scene_pos.z > 0.0f) {
                        hi = mid;
                    } else {
                        lo = mid;
                    }
                }

                const float2 hit_uv = view_to_uv(hi);
                const float3 hit_color =
                    previous_color_texture.Sample(color_sampler, hit_uv).rgb;

                // Fade out as the hit approaches the screen edges (off-screen
                // data is unavailable) and as the ray length grows (less
                // reliable far hits).
                const float2 edge = smoothstep(0.0f, 0.15f, hit_uv) *
                    smoothstep(0.0f, 0.15f, 1.0f - hit_uv);
                const float edge_fade = edge.x * edge.y;
                const float distance_fade =
                    saturate(1.0f - (step_length * i) / max_distance);

                return float4(hit_color, edge_fade * distance_fade);
            }
            break;
        }

        previous_sample_pos = sample_pos;
    }

    return float4(0.0f, 0.0f, 0.0f, 0.0f);
}
