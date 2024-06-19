#version 460 core
layout(location = 0) in vec3 position;

layout(std140, binding = 0) uniform Transform
{
    mat4 model_matrix;
    mat4 view_matrix;
    mat4 projection_matrix;
};

layout(std430, binding = 1) buffer ColorBuffer
{
    vec3 colors[];
};

out vec3 color_out;

void main()
{
    gl_Position = projection_matrix * view_matrix * vec4(position, 1.0);

    // Each line is made up of two vertices,
    // so we need to divide by 2 to get the correct color index
    color_out = colors[gl_VertexID / 2];
}
