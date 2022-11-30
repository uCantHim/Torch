// Animation related stuff

#ifndef TRC_MATUTILS_ANIMATION_CALCULATIONS_H
#define TRC_MATUTILS_ANIMATION_CALCULATIONS_H

#define animMeta $animationMetaDataDescriptorName
#define animations $animationDataDescriptorName
#define vertexBoneIndices $vertexBoneIndicesAttribName
#define vertexBoneWeights $vertexBoneWeightsAttribName

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

#endif
