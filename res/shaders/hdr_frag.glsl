#version 460 core

out vec4 frag_color;

in vec2 tex_coords_out;

uniform float exposure;
uniform sampler2D hdr_framebuffer;

void main()
{
    const float gamma = 2.2;
    vec3 hdr_color = texture(hdr_framebuffer, tex_coords_out).rgb;
    vec3 mapped = vec3(1.0) - exp(-hdr_color * exposure);
    mapped = pow(mapped, vec3(1.0 / gamma));
    frag_color = vec4(mapped, 1.0);
}
