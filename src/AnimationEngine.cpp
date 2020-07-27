#include "AnimationEngine.h"



trc::AnimationEngine::AnimationEngine(const Rig& rig)
    :
    rig(&rig)
{
}

void trc::AnimationEngine::playAnimation(ui32 index)
{
    currentAnimation = &rig->getAnimation(index);
    currentAnimationIndex = currentAnimation->getBufferIndex();
}

void trc::AnimationEngine::playAnimation(const std::string& name)
{
    currentAnimation = &rig->getAnimationByName(name);
    currentAnimationIndex = currentAnimation->getBufferIndex();
}

auto trc::AnimationEngine::getCurrentAnimationIndex() const noexcept -> ui32
{
    return currentAnimationIndex;
}

auto trc::AnimationEngine::getCurrentFrameWeight() noexcept -> float
{
    if (currentAnimation == nullptr) {
        return 0.0f;
    }

    assert(currentAnimation->getFrameTime() != 0.0f);
    float frameWeight = keyframeTimer.duration() / currentAnimation->getFrameTime();

    if (frameWeight > 1.0f)
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

    return frameWeight;
}

auto trc::AnimationEngine::getCurrentFrames() noexcept -> uvec2
{
    return currentFrames;
}

void trc::AnimationEngine::pushConstants(
    ui32 offset,
    vk::PipelineLayout layout,
    vk::CommandBuffer cmdBuf)
{
    auto stage = vk::ShaderStageFlagBits::eVertex;
    cmdBuf.pushConstants<ui32>(layout, stage, offset, currentAnimationIndex);
    cmdBuf.pushConstants<uvec2>(layout, stage, offset + 4, getCurrentFrames());
    cmdBuf.pushConstants<float>(layout, stage, offset + 12, getCurrentFrameWeight());
}
