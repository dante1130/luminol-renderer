#version 460 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 tex_coords;

out vec2 tex_coords_out;

layout(std140, binding = 0) uniform Transform
{
    mat4 model_matrix;
    mat4 projection_matrix;
};

void main()
{
    gl_Position = projection_matrix * model_matrix * vec4(position, 1.0);
    tex_coords_out = tex_coords;
}
