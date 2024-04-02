#version 460 core

out vec4 frag_color;

in vec2 tex_coords_out;

uniform vec3 color;
uniform sampler2D texture_diffuse;

void main()
{
    frag_color = texture(texture_diffuse, tex_coords_out) * vec4(color, 1.0);
}
