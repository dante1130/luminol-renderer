#version 460 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 tex_coords;

out vec2 tex_coords_out;
out vec3 color_out;

layout(std140, binding = 0) uniform Transform
{
    mat4 model_matrix;
    mat4 view_matrix;
    mat4 projection_matrix;
};

layout(std430, binding = 0) buffer InstancingModelMatrix
{
    mat4 model_matrices[];
};

layout(std430, binding = 1) buffer ColorBuffer
{
    vec3 colors[];
};

void main()
{
    gl_Position = projection_matrix * view_matrix * model_matrices[gl_InstanceID] * vec4(position, 1.0);
    tex_coords_out = tex_coords;
    color_out = colors[gl_InstanceID];
}
