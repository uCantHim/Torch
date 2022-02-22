// Animation related stuff

#include "asset_registry_descriptor.glsl"

#ifndef BONE_INDICES_INPUT_LOCATION
#define BONE_INDICES_INPUT_LOCATION 4
#endif

#ifndef BONE_WEIGHTS_INPUT_LOCATION
#define BONE_WEIGHTS_INPUT_LOCATION 5
#endif

#define NO_ANIMATION (uint(0) - 1)

struct AnimationPushConstantData
{
    uint animation;
    uint keyframes[2];
    float keyframeWeigth;
};

layout (location = BONE_INDICES_INPUT_LOCATION) in uvec4 vertexBoneIndices;
layout (location = BONE_WEIGHTS_INPUT_LOCATION) in vec4 vertexBoneWeights;


vec4 applyAnimation(uint animIndex, vec4 vertPos, uint frames[2], float frameWeight)
{
    vec4 currentFramePos = vec4(0.0);
    vec4 nextFramePos = vec4(0.0);

    const uint baseOffset = animMeta.metas[animIndex].baseOffset;
    const uint boneCount = animMeta.metas[animIndex].boneCount;

    for (int i = 0; i < 4; i++)
    {
        const float weight = vertexBoneWeights[i];
        if (weight <= 0.0) {
            break;
        }

        uint boneIndex = vertexBoneIndices[i];
        uint currentFrameOffset = baseOffset + boneCount * frames[0] + boneIndex;
        uint nextFrameOffset = baseOffset + boneCount * frames[1] + boneIndex;

        mat4 currentBoneMatrix = animations.boneMatrices[currentFrameOffset];
        mat4 nextBoneMatrix = animations.boneMatrices[nextFrameOffset];

        currentFramePos += currentBoneMatrix * vertPos * weight;
        nextFramePos += nextBoneMatrix * vertPos * weight;
    }

    return currentFramePos * (1.0 - frameWeight) + nextFramePos * frameWeight;
}
