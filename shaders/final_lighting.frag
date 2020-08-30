#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

#include "material.glsl"
#include "light.glsl"

layout (input_attachment_index = 0, set = 2, binding = 0) uniform subpassInput vertexPosition;
layout (input_attachment_index = 1, set = 2, binding = 1) uniform subpassInput vertexNormal;
layout (input_attachment_index = 2, set = 2, binding = 2) uniform subpassInput vertexUv;
layout (input_attachment_index = 3, set = 2, binding = 3) uniform subpassInput materialIndex;

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

layout (push_constant) uniform PushConstants
{
    vec3 cameraPos;
} pushConstants;

layout (location = 0) out vec4 fragColor;


/////////////////////
//      Main       //
/////////////////////

#define SHADOW_DESCRIPTOR_SET_BINDING 4
#include "lighting.glsl"

void main()
{
    vec3 color = vec3(0.3, 1.0, 0.9);
    const uint matIndex = floatBitsToUint(subpassLoad(materialIndex).r);

    // Use diffuse texture if available
    uint diffTexture = materials[matIndex].diffuseTexture;
    if (diffTexture != NO_TEXTURE)
    {
        vec2 uv = subpassLoad(vertexUv).xy;
        color = texture(textures[diffTexture], uv).rgb;
    }

    fragColor = vec4(
        calcLighting(
            color,
            subpassLoad(vertexPosition).xyz,
            normalize(subpassLoad(vertexNormal).xyz),
            pushConstants.cameraPos,
            matIndex
        ),
        1.0
    );

    // Exchange doesn't seem to have any difference in performance to imageLoad().
    // Use exchange to reset head pointer to default value.
    uint fragListIndex = imageAtomicExchange(fragmentListHeadPointer, ivec2(gl_FragCoord.xy), ~0u);
    while (fragListIndex != ~0u)
    {
        fragColor *= unpackUnorm4x8(fragmentList[fragListIndex][0]);
        fragListIndex = fragmentList[fragListIndex][2];
    }
}
