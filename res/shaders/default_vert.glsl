#version 460 core

layout(location = 0) in vec3 position;

out vec3 frag_pos;

void main()
{
    gl_Position = vec4(position, 1.0);
    frag_pos = position;
}
