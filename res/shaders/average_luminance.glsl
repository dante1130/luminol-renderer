#version 460 core
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
layout(rgba16f, binding = 2) uniform image2D average_luminance_image;

layout(std430, binding = 2) buffer LuminanceHistogramBuffer
{
    uint luminance_histogram[256];
};

uniform float delta_time;
uniform int num_pixels;

void main()
{
    const float min_log_luminance = -10.0;
    const float max_log_luminance = 2.0;
    const float log_luminance_range = abs(min_log_luminance) + max_log_luminance;

    uint total_weighted_sum = 0u;
    uint black_pixel_count = luminance_histogram[0];

    for (uint i = 1u; i < 255u; ++i)
    {
        total_weighted_sum += luminance_histogram[i] * i;
    }

    const float weighted_log_average = (float(total_weighted_sum) / max(float(num_pixels) - float(black_pixel_count), 1.0)) - 1.0;
    const float weighted_average_luminance = exp2(((weighted_log_average / 254.0) * log_luminance_range) + min_log_luminance);

    const float luminance_last_frame = imageLoad(average_luminance_image, ivec2(0, 0)).r;

    const float tau = 2.0;
    const float time_coefficient = 1.0 - exp2(-delta_time * tau);

    const float adapted_luminance = luminance_last_frame + (weighted_average_luminance - luminance_last_frame) * time_coefficient;

    imageStore(average_luminance_image, ivec2(0, 0), vec4(adapted_luminance, 0.0, 0.0, 0.0));
}
