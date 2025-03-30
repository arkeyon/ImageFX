#version 450

layout(location = 0) in struct {
    vec4 color;
    vec2 texCoord;
} In;

layout(binding = 0) uniform sampler2D u_FontAtlasTexture;

layout(location = 0) out vec4 fragColor;

void main()
{
    fragColor = vec4(texture(u_FontAtlasTexture, In.texCoord).r) * In.color;
}