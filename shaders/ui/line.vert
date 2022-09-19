#version 460

layout (location = 0) in vec2 vertexUv;

layout (location = 1) in vec2 start;
layout (location = 2) in vec2 end;
layout (location = 3) in vec4 color;

layout (location = 0) out vec4 outColor;

void main()
{
    const vec2 pos = start + (end - start) * vertexUv;
    gl_Position = vec4(pos * 2.0 - 1.0, 0.0, 1.0);
    outColor = color;
}
