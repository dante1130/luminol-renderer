#version 460 core

out vec4 frag_color;

in vec2 tex_coords_out;

uniform sampler2D hdr_framebuffer;

void main()
{
    frag_color = texture(hdr_framebuffer, tex_coords_out);
}
