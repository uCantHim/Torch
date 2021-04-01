#version 460

layout (location = 0) in Vertex
{
    vec4 pos;
    vec3 normal;
    vec2 uv;
    flat uint material;
} vert;

layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec2 outUv;
layout (location = 3) out uint outMaterial;

void main()
{
    outPosition = vert.pos;
    outNormal = vert.normal;
    outUv = vert.uv;
    outMaterial = vert.material;
}
