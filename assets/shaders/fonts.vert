#version 450

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec2 aTexCoord;

out vec4 color;
out vec2 texCoord;

uniform mat4 uViewProjectionMat;

void main()
{
    gl_Position = uViewProjectionMat * vec4(aPosition, 1.0);

    color = aColor;
    texCoord = aTexCoord;
}