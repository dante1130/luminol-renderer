#version 460 core
layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
layout(rgba16f, binding = 1) uniform image2D hdr_image;

const uint group_size = gl_WorkGroupSize.x * gl_WorkGroupSize.y;

layout(std430, binding = 2) buffer LuminanceHistogramBuffer
{
    uint luminance_histogram[group_size];
};

uint color_to_bin(vec3 color, float min_log_luminance, float inverse_log_luminance_range) {
    const vec3 rgb_to_luminance = vec3(0.2126, 0.7152, 0.0722);
    const float luminance = dot(color, rgb_to_luminance);

    const float epsilon = 0.005;

    if (luminance < epsilon) {
        return 0;
    }

    const float normalized_log_luminance = clamp((log2(luminance) - min_log_luminance) * inverse_log_luminance_range, 0.0, 1.0);

    return uint(normalized_log_luminance * 254.0 + 1.0);
}

void main() {
    const ivec2 texel = ivec2(gl_GlobalInvocationID.xy);

    const vec3 color = imageLoad(hdr_image, texel).rgb;

    const float min_log_luminance = -10.0;
    const float max_log_luminance = 2.0;
    const float log_luminance_range = abs(min_log_luminance) + max_log_luminance;
    const float inverse_log_luminance_range = 1.0 / log_luminance_range;

    const uint bin = color_to_bin(color, min_log_luminance, inverse_log_luminance_range);

    atomicAdd(luminance_histogram[bin], 1);
}
