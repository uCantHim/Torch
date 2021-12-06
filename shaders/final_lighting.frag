#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

#define ASSET_DESCRIPTOR_SET_BINDING 1
#include "asset_registry_descriptor.glsl"

#define SHADOW_DESCRIPTOR_SET_BINDING 4
#define LIGHT_DESCRIPTOR_SET 3
#define LIGHT_DESCRIPTOR_BINDING 0
#include "lighting.glsl"

#define MAX_FRAGS 10

layout (input_attachment_index = 0, set = 2, binding = 0) uniform subpassInput inNormal;
layout (input_attachment_index = 1, set = 2, binding = 1) uniform usubpassInput inAlbedo;
layout (input_attachment_index = 2, set = 2, binding = 2) uniform usubpassInput inMaterial;
layout (input_attachment_index = 3, set = 2, binding = 3) uniform subpassInput inDepth;

layout (set = 0, binding = 0, std140) restrict uniform CameraBuffer
{
    mat4 viewMatrix;
    mat4 projMatrix;
    mat4 inverseViewMatrix;
    mat4 inverseProjMatrix;
} camera;

layout (set = 0, binding = 1) restrict readonly uniform GlobalDataBuffer
{
    vec2 mousePos;
    vec2 resolution;
} global;

layout (set = 2, binding = 4, r32ui) uniform uimage2D fragmentListHeadPointer;

layout (set = 2, binding = 6) restrict buffer FragmentList
{
    /**
     * 0: A packed color
     * 1: Fragment depth value
     * 2: Next-pointer
     */
    uvec4 fragmentList[];
};

layout (location = 0) out vec4 fragColor;


/////////////////////
//      Main       //
/////////////////////

vec4 worldPosFromDepth(float depth);
vec3 blendTransparent(vec3 opaqueColor);

void main()
{
    const uint matIndex = subpassLoad(inMaterial)[0];
    vec3 color = unpackUnorm4x8(subpassLoad(inAlbedo)[0]).xyz;

    color = calcLighting(
        color,
        worldPosFromDepth(subpassLoad(inDepth).x).xyz,
        normalize(subpassLoad(inNormal).xyz),
        camera.inverseViewMatrix[3].xyz,
        matIndex
    );

    color = blendTransparent(color);

    fragColor = vec4(color, 1.0);
}


vec4 worldPosFromDepth(float depth)
{
    vec2 fc = gl_FragCoord.xy;
#ifdef TRC_FLIP_Y_AXIS
    fc.y = global.resolution.y - fc.y;  // Viewport height inversion
#endif

    const vec4 clipSpace = vec4(fc / global.resolution * 2.0 - 1.0, depth, 1.0);
    const vec4 viewSpace = camera.inverseProjMatrix * clipSpace;
    const vec4 worldSpace = camera.inverseViewMatrix * (viewSpace / viewSpace.w);

    return worldSpace;
}


vec3 blendTransparent(vec3 opaqueColor)
{
    // Exchange doesn't seem to have any difference in performance to imageLoad().
    // Use exchange to reset head pointer to default value.
    uint fragListIndex = imageAtomicExchange(fragmentListHeadPointer, ivec2(gl_FragCoord.xy), ~0u);

    // Build fragment list
    uvec4 fragments[MAX_FRAGS];
    int numFragments = 0;
    while (numFragments < MAX_FRAGS && fragListIndex != ~0u)
    {
        fragments[numFragments] = fragmentList[fragListIndex];
        numFragments++;
        fragListIndex = fragmentList[fragListIndex].z;
    }

    // Sort fragment list
    for (int i = numFragments - 1; i > 0; i--)
    {
        for (int j = 0; j < i; j++)
        {
            float depth1 = uintBitsToFloat(fragments[j].y);
            float depth2 = uintBitsToFloat(fragments[j + 1].y);
            if (depth1 > depth2)
            {
                uvec4 temp = fragments[j];
                fragments[j] = fragments[j + 1];
                fragments[j + 1] = temp;
            }
        }
    }

    // Blend fragments
    for (int i = numFragments - 1; i >= 0; i--)
    {
        vec4 color = unpackUnorm4x8(fragments[i].x);
        opaqueColor = opaqueColor * (1.0 - color.a) + color.rgb * color.a;
    }

    return opaqueColor;
}
