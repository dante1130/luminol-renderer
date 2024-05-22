#version 460 core
layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
layout(rgba16f, binding = 1) uniform image2D hdr_image;

const uint group_size = gl_WorkGroupSize.x * gl_WorkGroupSize.y;

void main() {
    ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);
    vec4 color = imageLoad(hdr_image, pixel_coords);
    imageStore(hdr_image, pixel_coords, color);
}
