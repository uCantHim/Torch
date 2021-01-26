#version 460

layout (location = 0) in vec2 vertexPosition;
layout (location = 1) in vec2 quadPos;
layout (location = 2) in vec2 quadSize;
layout (location = 3) in vec4 quadColor;

layout (location = 0) out Vertex {
    vec4 color;
};

void main()
{
    const vec2 pos = (quadSize * vertexPosition) + quadPos;
    gl_Position = vec4(pos * 2.0 - 1.0, 0.0, 1.0);
    color = quadColor;
}
