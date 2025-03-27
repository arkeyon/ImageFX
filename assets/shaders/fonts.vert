#version 450

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec4 aColor;
layout(location = 2) in vec2 aTexCoord;

layout(location = 0) out vec4 color;
layout(location = 1) out vec2 texCoord;

layout(push_constant) uniform uPushConstant
{
    mat3 transform;
}

void main()
{
    gl_Position = vec4(aPosition, 1.0);

    color = aColor;
    texCoord = aTexCoord;
}