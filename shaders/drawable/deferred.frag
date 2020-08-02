#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

#include "../material.glsl"

layout (constant_id = 1) const bool isPickable = false;

layout (set = 0, binding = 1) restrict readonly uniform GlobalDataBuffer
{
    vec2 mousePos;
    vec2 resolution;
} global;

layout (set = 1, binding = 0, std430) restrict readonly buffer MaterialBuffer
{
    Material materials[];
};

layout (set = 1, binding = 1) uniform sampler2D textures[];

layout (location = 0) in Vertex
{
    vec3 worldPos;
    vec2 uv;
    flat uint material;
    mat3 tbn;

    flat uint pickableID;
    flat uint instanceIndex;
} vert;

layout (set = 2, binding = 1) restrict writeonly buffer PickingBuffer
{
    uint pickableID;
    uint instanceID;
} picking;

layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec2 outUv;
layout (location = 3) out uint outMaterial;


/////////////////////
//      Main       //
/////////////////////

void main()
{
    outPosition = vec4(vert.worldPos, 1.0);
    outUv = vert.uv;
    outMaterial = vert.material;

    uint bumpTex = materials[vert.material].bumpTexture;
    if (bumpTex == NO_TEXTURE)
    {
        outNormal = vert.tbn[2];
    }
    else
    {
        vec3 textureNormal = texture(textures[bumpTex], vert.uv).rgb * 2.0 - 1.0;
        textureNormal.y = -textureNormal.y;  // Vulkan axis flip
        outNormal = vert.tbn * textureNormal;
    }

    if (isPickable)
    {
        if (gl_FragCoord.xy == global.mousePos)
        {
            picking.pickableID = vert.pickableID;
            picking.instanceID = vert.instanceIndex;
        }
    }
}
