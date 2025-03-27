#version 450

layout(location = 0) in vec3 positions;
layout(location = 1) in vec4 colors;
layout(location = 2) in vec2 tex_coord;

layout(location = 0) out vec4 fragColor;

/*
vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.7, 0.5),
    vec2(-0.5, 0.5)
);

vec3 colors[3] = vec3[](
    vec3(0.2, 0.2, 0.2),
    vec3(1.0, 1.0, 1.0),
    vec3(0.7, 0.7, 0.7)
);*/

void main() {
    gl_Position = vec4(positions, 1.0);
    fragColor = colors;
}