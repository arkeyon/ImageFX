#version 450

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec4 a_Color;
layout(location = 2) in vec2 a_TexCoord;
layout(location = 3) in float a_SamplerID;

layout(location = 0) out struct {
    vec4 color;
    vec2 texCoord;
    float samplerid;
} Out;

layout(push_constant) uniform PushConstant
{
    mat4 projection_view;
    mat4 model;
} u_PC;

void main()
{
    gl_Position = u_PC.projection_view * u_PC.model * vec4(a_Position, 1.0);

    Out.color = a_Color;
    Out.texCoord = a_TexCoord;
    Out.samplerid = a_SamplerID;
}