#include "AnimationEngine.h"

#include "AssetManager.h"
#include "RigRegistry.h"



trc::AnimationEngine::AnimationEngine(RigDeviceHandle rig)
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
    if (!rig.has_value()) {
        throw std::runtime_error("[In AnimationEngine::playAnimation]: No rig is associated with"
                                 " this animation engine!");
    }

    try {
        currentAnimation = rig->getAnimation(index).getDeviceDataHandle();
        resetAnimationTime();
    }
    catch (const std::out_of_range& err) {
        // Do nothing
    }
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
