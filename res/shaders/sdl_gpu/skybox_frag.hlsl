TextureCube skybox_texture : register(t0, space2);
SamplerState skybox_sampler : register(s0, space2);

struct PSInput {
    float3 view_dir : TEXCOORD0;
};

float4 main(PSInput input) : SV_Target {
    return skybox_texture.Sample(skybox_sampler, input.view_dir);
}
