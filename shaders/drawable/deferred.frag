#version 460

layout (location = 0) in Vertex
{
    vec3 worldPos;
    vec3 normal;
    vec3 tangent;
    vec2 uv;
    flat uint material;
} vert;

layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec2 outUv;
layout (location = 3) out uint outMaterial; // Don't ask me why I have to specify float here


/////////////////////
//      Main       //
/////////////////////

void main()
{
    outPosition = vec4(vert.worldPos, 1.0);
    outNormal = vert.normal;
    outUv = vert.uv;
    outMaterial = vert.material;
}
