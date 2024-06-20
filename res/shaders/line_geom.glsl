#version 460 core
layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

in VS_OUT
{
    float line_width;
} gs_in[];

void main()
{
    const float line_width = gs_in[0].line_width;
    const vec4 line_start_position = gl_in[0].gl_Position;
    const vec4 line_end_position = gl_in[1].gl_Position;

    const vec3 line_direction = normalize(line_end_position.xyz - line_start_position.xyz);
    const vec3 line_normal = normalize(cross(line_direction, vec3(0.0, 1.0, 0.0)));

    const vec3 offset = line_normal * line_width;

    gl_Position = line_start_position + vec4(offset, 0.0);
    EmitVertex();
    gl_Position = line_end_position + vec4(offset, 0.0);
    EmitVertex();
    gl_Position = line_start_position - vec4(offset, 0.0);
    EmitVertex();
    gl_Position = line_end_position - vec4(offset, 0.0);
    EmitVertex();

    EndPrimitive();
}
