// Descriptor binding declarations for the asset descriptor

#ifndef TRC_MATUTILS_ANIMATION_DATA_H
#define TRC_MATUTILS_ANIMATION_DATA_H

const uint NO_ANIMATION = (uint(0) - 1);

struct AnimationMetaData
{
    uint baseOffset;
    uint frameCount;
    uint boneCount;
};

struct AnimationPushConstantData
{
    uint animation;
    uint keyframes[2];
    float keyframeWeigth;
};

#endif
