#version 460 core

layout(location = 0) out vec3 gbuffer_position;
layout(location = 1) out vec3 gbuffer_normal;
layout(location = 2) out vec3 gbuffer_emissive;
layout(location = 3) out vec3 gbuffer_albedo;

in vec2 tex_coords_out;
in vec3 frag_pos_out;
in vec3 normal_out;
in mat3 tangent_space_matrix_out;

struct Material {
    sampler2D texture_diffuse;
    sampler2D texture_emissive;
    sampler2D texture_normal;
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
    gbuffer_position = frag_pos_out;
    gbuffer_normal = calculate_normal(material.texture_normal, tex_coords_out, tangent_space_matrix_out);
    gbuffer_albedo = texture(material.texture_diffuse, tex_coords_out).rgb;
    gbuffer_emissive = texture(material.texture_emissive, tex_coords_out).rgb;
}
