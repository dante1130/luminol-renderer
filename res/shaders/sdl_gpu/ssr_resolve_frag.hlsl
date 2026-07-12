// Screen-space reflection resolve (denoise + silhouette anti-alias).
//
// Two decoupled blurs of the raw trace result:
//
//  * Colour is gathered over a SMALL radius, confidence-weighted (each
//    neighbour contributes its reflected colour weighted by its own hit
//    confidence). This denoises the jittered trace and fills grainy
//    low-confidence pixels from nearby confident hits, while keeping reflection
//    detail (wood grain, textures) crisp - a small kernel over a smooth region
//    barely changes it.
//
//  * Confidence (the reflection mask) is averaged over a WIDER radius. The mask
//    flips hard from 1 to 0 along a reflected silhouette; on a thin, edge-on
//    object stretched at a grazing angle that hard edge staircases into a
//    "comb". Smoothing the mask over a wider neighbourhood turns that hard edge
//    into a soft gradient - anti-aliasing the silhouette - without touching the
//    reflection colour.

Texture2D ssr_texture : register(t0, space2);
SamplerState ssr_sampler : register(s0, space2);

// xy = width, height.
cbuffer ResolveBuffer : register(b0, space3) {
    float4 viewport_size;
};

struct PSInput {
    float2 uv : TEXCOORD0;
};

static const int color_radius = 2;
static const int mask_radius = 4;

float4 main(PSInput input) : SV_Target {
    const float2 texel_size = 1.0f / viewport_size.xy;

    float3 color_sum = float3(0.0f, 0.0f, 0.0f);
    float color_weight_sum = 0.0f;
    float confidence_sum = 0.0f;
    float sample_count = 0.0f;

    for (int x = -mask_radius; x <= mask_radius; ++x) {
        for (int y = -mask_radius; y <= mask_radius; ++y) {
            const float2 offset = float2(float(x), float(y)) * texel_size;
            const float4 s = ssr_texture.Sample(ssr_sampler, input.uv + offset);

            // Wide mask average: smooths the silhouette edge.
            confidence_sum += s.a;
            sample_count += 1.0f;

            // Narrow colour gather: keeps reflection detail sharp.
            if (abs(x) <= color_radius && abs(y) <= color_radius) {
                color_sum += s.rgb * s.a;
                color_weight_sum += s.a;
            }
        }
    }

    const float3 color = color_weight_sum > 0.0f
        ? color_sum / color_weight_sum
        : float3(0.0f, 0.0f, 0.0f);
    const float confidence = confidence_sum / sample_count;

    return float4(color, confidence);
}
