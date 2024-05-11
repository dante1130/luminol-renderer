#version 460 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 tex_coords;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;

out vec2 tex_coords_out;
out vec3 frag_pos_out;
out vec3 normal_out;
out mat3 tangent_space_matrix_out;

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

void main()
{
    gl_Position = projection_matrix * view_matrix * model_matrices[gl_InstanceID] * vec4(position, 1.0);
    frag_pos_out = vec3(model_matrix * vec4(position, 1.0));
    tex_coords_out = tex_coords;
    normal_out = mat3(transpose(inverse(model_matrix))) * normal;

    vec3 T = normalize(vec3(model_matrix * vec4(tangent, 0.0)));
    vec3 N = normalize(vec3(model_matrix * vec4(normal, 0.0)));
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);
    tangent_space_matrix_out = mat3(T, B, N);
}
