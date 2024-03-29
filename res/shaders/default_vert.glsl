#version 460 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 tex_coords;

out vec3 frag_pos;
out vec2 tex_coords_out;

void main()
{
    gl_Position = vec4(position, 1.0);
    frag_pos = position;
    tex_coords_out = tex_coords;
}
