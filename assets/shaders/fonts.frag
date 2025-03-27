#version 450

layout(location = 0) in vec4 color;
layout(location = 1) in vec2 texCoord;

layout(binding = 0) uniform sampler2D uFontAtlasTexture;

layout(location = 0) out vec4 fragColor;

void main()
{
    fragColor = vec4(texture(uFontAtlasTexture, texCoord).r) * color;
}