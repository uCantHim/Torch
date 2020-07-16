#version 460

layout (location = 0) out vec4 fragColor;

layout (location = 0) in Vertex
{
    vec3 normal;
    vec2 uv;
} vert;

void main()
{
    fragColor = vec4(abs(vert.normal), 1.0);
}
