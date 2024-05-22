#version 460 core
layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
layout(rgba16f, binding = 2) uniform image2D average_luminance_image;

const uint group_size = gl_WorkGroupSize.x * gl_WorkGroupSize.y;

layout(std430, binding = 2) buffer LuminanceHistogramBuffer
{
    uint luminance_histogram[group_size];
};

uniform float delta_time;
uniform int num_pixels;

shared uint shared_luminance_histogram[group_size];

void main()
{
    const uint local_index = gl_LocalInvocationIndex;

    const uint luminance_count = luminance_histogram[local_index];
    shared_luminance_histogram[local_index] = luminance_count * local_index;

    barrier();

    for (uint stride = group_size / 2; stride > 0; stride /= 2)
    {
        if (local_index < stride)
        {
            shared_luminance_histogram[local_index] += shared_luminance_histogram[local_index + stride];
        }

        barrier();
    }

    if (local_index == 0)
    {
        const float min_log_luminance = -10.0;
        const float max_log_luminance = 2.0;
        const float log_luminance_range = abs(min_log_luminance) + max_log_luminance;

        const float weighted_log_average = (shared_luminance_histogram[0] / max(num_pixels - float(luminance_count), 1.0)) - 1.0;
        const float weighted_average_luminance = exp2(((weighted_log_average / 254.0) * log_luminance_range) + min_log_luminance);

        const float luminance_last_frame = imageLoad(average_luminance_image, ivec2(0, 0)).r;

        const float tau = 1.1;
        const float time_coefficient = 1.0 - exp2(-delta_time * tau);

        const float adapted_luminance = luminance_last_frame + (weighted_average_luminance - luminance_last_frame) * time_coefficient;

        imageStore(average_luminance_image, ivec2(0, 0), vec4(adapted_luminance, 0.0, 0.0, 0.0));
    }
}
