#version 460

layout (set = 1, binding = 1) uniform sampler2D textures[];

layout (location = 0) in Vertex
{
    vec3 worldPos;
    vec2 uv;
    vec3 normal;
} vert;

layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec2 outUv;
layout (location = 3) out uint outMaterial;

void main()
{
    outPosition = vec4(vert.worldPos, gl_FragCoord.z);
    outNormal = normalize(vert.normal);
    outUv = vert.uv;
    outMaterial = 0;
}
