#version 460 core
layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
layout(rgba16f, binding = 1) uniform image2D hdr_image;

const uint group_size = gl_WorkGroupSize.x * gl_WorkGroupSize.y;

const vec3 rgb_to_luminance = vec3(0.2126, 0.7152, 0.0722);

layout(std430, binding = 2) buffer LuminanceHistogramBuffer
{
    uint luminance_histogram[group_size];
};

void main() {
    const ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);
    const vec4 color = imageLoad(hdr_image, pixel_coords);
    const float luminance = dot(color.rgb, rgb_to_luminance);
    const uint bin = uint(luminance * float(group_size));
    atomicAdd(luminance_histogram[bin], 1);
}
