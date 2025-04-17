#version 450

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec4 a_Color;

layout(location = 0) out vec4 o_color;

layout(push_constant) uniform PushConstant
{
    mat4 projection_view;
    mat4 model;
} u_PC;

void main()
{
    gl_Position = u_PC.projection_view * u_PC.model * vec4(a_Position, 1.0);

    o_color = a_Color;
}