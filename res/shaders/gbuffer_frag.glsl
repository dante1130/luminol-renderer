#version 460 core

layout(location = 0) out vec4 gbuffer_position_metallic;
layout(location = 1) out vec4 gbuffer_normal_roughness;
layout(location = 2) out vec4 gbuffer_emissive_ao;
layout(location = 3) out vec3 gbuffer_albedo;

in vec2 tex_coords_out;
in vec3 frag_pos_out;
in vec3 normal_out;
in mat3 tangent_space_matrix_out;

struct Material {
    sampler2D texture_diffuse;
    sampler2D texture_emissive;
    sampler2D texture_normal;
    sampler2D texture_metallic;
    sampler2D texture_roughness;
    sampler2D texture_ao;
};

uniform Material material;

vec3 calculate_normal(sampler2D material_texture_normal, vec2 tex_coords, mat3 tbn)
{
    vec3 normal_map = texture(material_texture_normal, tex_coords).rgb;
    normal_map = normalize(normal_map * 2.0 - 1.0);

    return normalize(tbn * normal_map);
}

void main()
{
    gbuffer_position_metallic.rgb = frag_pos_out;
    gbuffer_position_metallic.a = texture(material.texture_metallic, tex_coords_out).b;

    gbuffer_normal_roughness.rgb = calculate_normal(material.texture_normal, tex_coords_out, tangent_space_matrix_out);
    gbuffer_normal_roughness.a = texture(material.texture_roughness, tex_coords_out).g;

    gbuffer_emissive_ao.rgb = texture(material.texture_emissive, tex_coords_out).rgb;
    gbuffer_emissive_ao.a = texture(material.texture_ao, tex_coords_out).r;

    gbuffer_albedo = texture(material.texture_diffuse, tex_coords_out).rgb;
}
