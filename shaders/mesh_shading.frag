#version 460

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec4 outAlbedo;
layout (location = 2) out uint outMaterial;

layout (location = 0) in VertexData
{
    vec3 normal;
    vec3 color;
    flat uint material;
} vertexIn;

void main()
{
    outNormal = vertexIn.normal;
    outAlbedo = vec4(vertexIn.color, 1.0f);
    outMaterial = vertexIn.material;
}
