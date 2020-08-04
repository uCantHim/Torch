#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

#include "../material.glsl"

// Constants
layout (constant_id = 1) const bool isPickable = false;

// Buffers
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

layout (set = 2, binding = 1) restrict buffer PickingBuffer
{
    uint pickableID;
    uint instanceID;
    float depth;
} picking;

// Push Constants
layout (push_constant) uniform PushConstants
{
    layout (offset = 84) uint pickableID;
};

// Input
layout (location = 0) in Vertex
{
    vec3 worldPos;
    vec2 uv;
    flat uint material;
    mat3 tbn;

    flat uint instanceIndex;
} vert;

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

    if (isPickable) // constant
    {
        // Floor because Vulkan sets the frag coord to .5 (the middle of the pixel)
        if (floor(gl_FragCoord.xy) == global.mousePos
            && gl_FragCoord.z < picking.depth)
        {
            picking.pickableID = pickableID;
            picking.instanceID = vert.instanceIndex;
            picking.depth = gl_FragCoord.z;
        }
    }
}
