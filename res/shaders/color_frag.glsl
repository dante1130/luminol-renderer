#version 460 core

out vec4 frag_color;

in vec2 tex_coords_out;
in vec3 color_out;

uniform sampler2D texture_diffuse;

void main()
{
    frag_color = texture(texture_diffuse, tex_coords_out) * vec4(color_out, 1.0);
}
