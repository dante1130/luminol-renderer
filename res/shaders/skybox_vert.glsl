#version 460 core

layout(location = 0) in vec3 position;

out vec3 tex_coords_out;

layout(std140, binding = 0) uniform Transform
{
    mat4 model_matrix;
    mat4 view_matrix;
    mat4 projection_matrix;
};

void main()
{
    vec4 pos = projection_matrix * view_matrix * vec4(position, 1.0);
    gl_Position = pos.xyww;
    tex_coords_out = position;
}
