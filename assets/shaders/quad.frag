#version 450

layout(location = 0) in  vec4 i_color;

layout(location = 0) out vec4 fragColor;

void main()
{
    fragColor = i_color;
}