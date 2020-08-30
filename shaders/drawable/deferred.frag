#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

#include "../material.glsl"

//layout (early_fragment_tests) in;

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

layout (set = 3, binding = 4, r32ui) uniform uimage2D fragmentListHeadPointer;

layout (set = 3, binding = 5) restrict buffer FragmentListAllocator
{
    uint nextFragmentListIndex;
    uint maxFragmentListIndex;
};

layout (set = 3, binding = 6) restrict buffer FragmentList
{
    /**
     * 0: A packed color
     * 1: Fragment depth value
     * 2: Next-pointer
     */
    uint fragmentList[][3];
};

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

void appendFragment(vec4 color);

void main()
{
    uint diffTex = materials[vert.material].diffuseTexture;
    vec4 diffTexColor = texture(textures[diffTex], vert.uv);
    if (diffTexColor.a < 1.0)
    {
        appendFragment(vec4(0, 0, 1, 1));
        discard;
    }

    outPosition = vec4(vert.worldPos, gl_FragCoord.z);
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


void appendFragment(vec4 color)
{
    uint newIndex = atomicAdd(nextFragmentListIndex, 1);
    if (newIndex > maxFragmentListIndex) {
        return;
    }

    uint newElement[3] = {
        packUnorm4x8(color),
        floatBitsToUint(gl_FragCoord.z - 0.001),
        imageAtomicExchange(fragmentListHeadPointer, ivec2(gl_FragCoord.xy), newIndex)
    };
    fragmentList[newIndex] = newElement;
}
