#version 460 core

out vec4 frag_color;

in vec2 tex_coords_out;

uniform sampler2D hdr_framebuffer;

const float min_log_luminance = -10.0;
const float max_log_luminance = 2.0;
const float log_luminance_range = abs(min_log_luminance) + max_log_luminance;
const float inverse_log_luminance_range = 1.0 / log_luminance_range;

vec3 jet_colormap(float t) {
    t = clamp(t, 0.0, 1.0);
    
    vec3 color;
    if (t < 0.25) {
        color = vec3(0.0, 4.0 * t, 1.0);
    } else if (t < 0.5) {
        color = vec3(0.0, 1.0, 1.0 - 4.0 * (t - 0.25));
    } else if (t < 0.75) {
        color = vec3(4.0 * (t - 0.5), 1.0, 0.0);
    } else {
        color = vec3(1.0, 1.0 - 4.0 * (t - 0.75), 0.0);
    }
    
    return clamp(color, 0.0, 1.0);
}

void main() {
    vec3 hdr_color = texture(hdr_framebuffer, tex_coords_out).rgb;
    
    const vec3 rgb_to_luminance = vec3(0.2126, 0.7152, 0.0722);
    float luminance = dot(hdr_color, rgb_to_luminance);
    
    const float epsilon = 0.005;
    float bin_normalized = 0.0;
    
    if (luminance >= epsilon) {
        bin_normalized = clamp(
            (log2(luminance) - min_log_luminance) * inverse_log_luminance_range,
            0.0,
            1.0
        );
    }
    
    frag_color = vec4(jet_colormap(bin_normalized), 0.7);
}
