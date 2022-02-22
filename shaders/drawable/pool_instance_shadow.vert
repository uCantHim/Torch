#version 460
#extension GL_GOOGLE_include_directive : require

#define BONE_INDICES_INPUT_LOCATION 4
#define BONE_WEIGHTS_INPUT_LOCATION 5
#define ASSET_DESCRIPTOR_SET_BINDING 1
#include "../animation.glsl"

layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec3 vertexNormal;
layout (location = 2) in vec2 vertexUv;
layout (location = 3) in vec3 vertexTangent;

layout (location = 6) in mat4 instanceModelMatrix;
layout (location = 10) in uvec4 instanceAnimData;

layout (set = 0, binding = 0, std430) buffer ShadowMatrices
{
    // These are the view-proj matrices
    mat4 shadowMatrices[];
};

layout (push_constant) uniform PushConstants
{
    uint lightIndex;  // Index into shadow matrix buffer
};

void main()
{
    vec4 vertPos = vec4(vertexPosition, 1.0);

    const uint animation = instanceAnimData[0];
    if (animation != NO_ANIMATION)
    {
        const uint keyframes[2] = { instanceAnimData[1], instanceAnimData[2] };
        const float weight = uintBitsToFloat(instanceAnimData[3]);
        vertPos = applyAnimation(animation, vertPos, keyframes, weight);
    }
    vertPos.w = 1.0;

    mat4 viewProj = shadowMatrices[lightIndex];
    gl_Position = viewProj * instanceModelMatrix * vertPos;
}
