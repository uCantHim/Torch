#version 460

layout (location = 0) in Vertex {
    vec4 color;
} vert;

layout (location = 0) out vec4 fragColor;

void main()
{
    fragColor = vert.color;
}
