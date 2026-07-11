// Debug-only: renders the Hi-Z pyramid's mip 0 straight to the screen as
// grayscale, to visually sanity-check it against the real depth buffer's
// silhouette. Not used in normal rendering.

Texture2D hiz_pyramid : register(t0, space2);
SamplerState hiz_sampler : register(s0, space2);

// Standard non-reversed depth (near=0, far=1) is heavily compressed near the
// far plane, so a raw linear visualization is almost all-white except right
// next to the camera. Linearize back to view-space Z and normalize across
// [near_plane, far_plane] so the whole range is visible.
cbuffer DebugVisualizeParams : register(b0, space3) {
    float near_plane;
    float far_plane;
    float2 padding;
};

struct PSInput {
    float2 uv : TEXCOORD0;
};

float4 main(PSInput input) : SV_Target {
    float depth = hiz_pyramid.SampleLevel(hiz_sampler, input.uv, 0.0).r;
    float linear_z =
        (near_plane * far_plane) / (far_plane - depth * (far_plane - near_plane));
    float normalized = saturate((linear_z - near_plane) / (far_plane - near_plane));
    return float4(normalized.xxx, 1.0);
}
