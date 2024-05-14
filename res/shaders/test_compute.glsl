#version 460 core
layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
layout(rgba32f, binding = 0) uniform image2D screen_image;

void main() {
    ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);
    vec4 color = vec4(1.0, 0.0, 0.0, 1.0);
    imageStore(screen_image, pixel_coords, color);
}