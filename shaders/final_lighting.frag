#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

#include "material.glsl"
#include "light.glsl"

#define MAX_FRAGS 10

layout (input_attachment_index = 0, set = 2, binding = 0) uniform subpassInput vertexPosition;
layout (input_attachment_index = 1, set = 2, binding = 1) uniform subpassInput vertexNormal;
layout (input_attachment_index = 2, set = 2, binding = 2) uniform subpassInput vertexUv;
layout (input_attachment_index = 3, set = 2, binding = 3) uniform usubpassInput materialIndex;

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

layout (set = 1, binding = 0, std430) restrict readonly buffer MaterialBuffer
{
    Material materials[];
};

layout (set = 1, binding = 1) uniform sampler2D textures[];

layout (set = 2, binding = 4, r32ui) uniform uimage2D fragmentListHeadPointer;

layout (set = 2, binding = 5) restrict buffer FragmentListAllocator
{
    uint nextFragmentListIndex;
};

layout (set = 2, binding = 6) restrict buffer FragmentList
{
    /**
     * 0: A packed color
     * 1: Fragment depth value
     * 2: Next-pointer
     */
    uvec4 fragmentList[];
};

layout (set = 3, binding = 0) restrict readonly buffer LightBuffer
{
    uint numLights;
    Light lights[];
};

layout (location = 0) out vec4 fragColor;


/////////////////////
//      Main       //
/////////////////////

#define SHADOW_DESCRIPTOR_SET_BINDING 4
#include "lighting.glsl"

vec3 blendTransparent(vec3 opaqueColor);

void main()
{
    vec3 color = vec3(0.3, 1.0, 0.9);
    const uint matIndex = subpassLoad(materialIndex).r;

    // Use diffuse texture if available
    uint diffTexture = materials[matIndex].diffuseTexture;
    if (diffTexture != NO_TEXTURE)
    {
        vec2 uv = subpassLoad(vertexUv).xy;
        color = texture(textures[diffTexture], uv).rgb;
    }

    color = calcLighting(
        color,
        subpassLoad(vertexPosition).xyz,
        normalize(subpassLoad(vertexNormal).xyz),
        camera.inverseViewMatrix[3].xyz,
        matIndex
    );

    color = blendTransparent(color);

    fragColor = vec4(color, 1.0);
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
