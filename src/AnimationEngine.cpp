#include "AnimationEngine.h"



trc::AnimationEngine::AnimationEngine(const Rig& rig)
    :
    rig(&rig)
{
}

void trc::AnimationEngine::update()
{
    if (currentAnimation == nullptr) {
        animationState.set({});
        return;
    }

    assert(currentAnimation->getFrameTime() != 0.0f);

    float frameWeight = keyframeTimer.duration() / currentAnimation->getFrameTime();
    if (frameWeight >= 1.0f)
    {
        frameWeight = 0.0f;
        keyframeTimer.reset();

        currentFrames += 1;
        if (currentFrames.x >= currentAnimation->getFrameCount()) {
            currentFrames.x = 0;
        }
        if (currentFrames.y >= currentAnimation->getFrameCount()) {
            currentFrames.y = 0;
        }
    }

    animationState.set({
        .currentAnimation = currentAnimation ? currentAnimation->getBufferIndex() : NO_ANIMATION,
        .keyframes = { currentFrames.x, currentFrames.y },
        .keyframeWeight = frameWeight,
    });
}

void trc::AnimationEngine::playAnimation(ui32 index)
{
    if (rig == nullptr) {
        throw std::runtime_error("[In AnimationEngine::playAnimation]: No rig is associated with"
                                 " this animation engine!");
    }

    try {
        currentAnimation = &rig->getAnimation(index);
    }
    catch (const std::out_of_range& err) {
        // Do nothing
    }
}

void trc::AnimationEngine::playAnimation(const std::string& name)
{
    if (rig == nullptr) {
        throw std::runtime_error("[In AnimationEngine::playAnimation]: No rig is associated with"
                                 " this animation engine!");
    }

    try {
        playAnimation(rig->getAnimationIndex(name));
    }
    catch (const std::out_of_range& err) {
        // Do nothing
    }
}

auto trc::AnimationEngine::getState() const -> ID
{
    return animationState;
}
