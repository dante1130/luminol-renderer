// Copies raw device depth from the (single-sample) scene depth texture into
// mip 0 of the Hi-Z pyramid (R32_Float). Paired with fullscreen_vert.hlsl.
// Depth formats can't be bound as compute storage/sampled textures on all
// backends, so this copy happens via a graphics pass instead of compute.

Texture2D depth_texture : register(t0, space2);
SamplerState depth_sampler : register(s0, space2);

struct PSInput {
    float2 uv : TEXCOORD0;
};

float main(PSInput input) : SV_Target {
    return depth_texture.Sample(depth_sampler, input.uv).r;
}
