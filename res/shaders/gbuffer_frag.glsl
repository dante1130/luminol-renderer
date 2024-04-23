#version 460 core

layout(location = 0) out vec4 gbuffer_position;
layout(location = 1) out vec4 gbuffer_normal;
layout(location = 2) out vec4 gbuffer_albedo_spec;

in vec2 tex_coords_out;
in vec3 frag_pos_out;
in vec3 normal_out;
in vec3 tangent_out;

struct Material {
    sampler2D texture_diffuse;
    sampler2D texture_emissive;
    sampler2D texture_specular;
    sampler2D texture_normal;
};

uniform Material material;

vec3 calculate_normal(sampler2D material_texture_normal, vec3 normal, vec3 tangent, vec2 tex_coords)
{
    vec3 normal_map = texture(material_texture_normal, tex_coords).rgb;
    normal_map = normalize(normal_map * 2.0 - 1.0);

    const vec3 n = normalize(normal);

    vec3 t = normalize(tangent);
    t = normalize(t - dot(t, n) * n);

    const vec3 b = cross(n, t);

    const mat3 tbn = mat3(t, b, n);

    return normalize(tbn * normal_map);
}

void main()
{
    gbuffer_position = vec4(frag_pos_out, 1.0);
    gbuffer_normal = vec4(calculate_normal(material.texture_normal, normal_out, tangent_out, tex_coords_out), 1.0);
    gbuffer_albedo_spec.rgb = texture(material.texture_diffuse, tex_coords_out).rgb + texture(material.texture_emissive, tex_coords_out).rgb;
    gbuffer_albedo_spec.a = texture(material.texture_specular, tex_coords_out).r;
}
