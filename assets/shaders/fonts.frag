#version 450

layout(location = 0) in struct {
    vec4 color;
    vec2 texCoord;
    float samplerid;
} In;

layout(binding = 0) uniform sampler3D u_FontAtlasTextures;
//layout(binding = 0) uniform sampler2DArray u_FontAtlasTextures;

layout(location = 0) out vec4 fragColor;

void main()
{
    fragColor = vec4(texture(u_FontAtlasTextures, vec3(In.texCoord.xy, In.samplerid)).r) * In.color;
}