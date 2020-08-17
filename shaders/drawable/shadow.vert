#version 460
#extension GL_GOOGLE_include_directive : require

#define BONE_INDICES_INPUT_LOCATION 4
#define BONE_WEIGHTS_INPUT_LOCATION 5
#define ANIM_DESCRIPTOR_SET_BINDING 1
#include "../animation.glsl"

layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec3 vertexNormal;
layout (location = 2) in vec2 vertexUv;
layout (location = 3) in vec3 vertexTangent;

layout (set = 0, binding = 0, std430) buffer ShadowMatrices
{
    // These are the view-proj matrices
    mat4 shadowMatrices[];
};

layout (push_constant) uniform PushConstants
{
    mat4 modelMatrix;
    uint lightIndex;  // Index into shadow matrix buffer

    uint animation;
    uint keyframes[2];
    float keyframeWeigth;
};

void main()
{
    mat4 viewProj = shadowMatrices[lightIndex];
    vec4 vertPos = vec4(vertexPosition, 1.0);
    if (animation != NO_ANIMATION) {
        vertPos = applyAnimation(animation, vertPos, keyframes, keyframeWeigth);
    }

    gl_Position = viewProj * modelMatrix * vertPos;
}
