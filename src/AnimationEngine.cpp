#include "trc/AnimationEngine.h"

#include "trc/base/Logging.h"



trc::AnimationEngine::AnimationEngine(RigHandle rig)
    :
    rig(rig)
{
}

void trc::AnimationEngine::update(const float timeDelta)
{
    if (!currentAnimation.has_value())
    {
        animationState.set({});
        return;
    }

    assert(currentAnimation->getFrameTime() != 0.0f);

    currentDuration += timeDelta;
    float frameWeight = currentDuration / currentAnimation->getFrameTime();
    if (frameWeight >= 1.0f)
    {
        frameWeight = 0.0f;
        currentDuration = 0.0f;

        currentFrames += 1;
        if (currentFrames.x >= currentAnimation->getFrameCount()) {
            currentFrames.x = 0;
        }
        if (currentFrames.y >= currentAnimation->getFrameCount()) {
            currentFrames.y = 0;
        }
    }

    animationState.set({
        .currentAnimation = currentAnimation->getBufferIndex(),
        .keyframes = { currentFrames.x, currentFrames.y },
        .keyframeWeight = frameWeight,
    });
}

void trc::AnimationEngine::playAnimation(ui32 index)
{
    try {
        currentAnimation = rig.getAnimation(index).getDeviceDataHandle();
        resetAnimationTime();
    }
    catch (const std::out_of_range& err) {
        log::warn << "Unable to play animation at index " << index << ": does not exist.";
    }
}

void trc::AnimationEngine::playAnimation(trc::AnimationHandle anim)
{
    currentAnimation = anim;
    resetAnimationTime();
}

auto trc::AnimationEngine::getState() const -> ID
{
    return animationState;
}

void trc::AnimationEngine::resetAnimationTime()
{
    currentFrames = { 0, 1 };
    currentDuration = 0.0f;
}
