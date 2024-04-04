#version 460 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 tex_coords;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;

out vec2 tex_coords_out;
out vec3 frag_pos_out;
out vec3 normal_out;

layout(std140, binding = 0) uniform Transform
{
    mat4 model_matrix;
    mat4 view_matrix;
    mat4 projection_matrix;
};

void main()
{
    gl_Position = projection_matrix * view_matrix * model_matrix * vec4(position, 1.0);
    frag_pos_out = vec3(model_matrix * vec4(position, 1.0));
    tex_coords_out = tex_coords;
    normal_out = mat3(transpose(inverse(model_matrix))) * normal;
}
