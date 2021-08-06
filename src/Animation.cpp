#include "Animation.h"

#include "AnimationDataStorage.h"



trc::Animation::Animation(ui32 animationIndex, const AnimationData& data)
    :
    id(animationIndex),
    frameCount(data.frameCount),
    durationMs(data.durationMs),
    frameTimeMs(data.frameTimeMs)
{
}

trc::Animation::Animation(AnimationDataStorage& storage, const AnimationData& data)
    :
    Animation(storage.makeAnimation(data))
{
}

auto trc::Animation::getBufferIndex() const noexcept -> ui32
{
    return id;
}

auto trc::Animation::getFrameCount() const noexcept -> ui32
{
    return frameCount;
}

auto trc::Animation::getDuration() const noexcept -> float
{
    return durationMs;
}

auto trc::Animation::getFrameTime() const noexcept -> float
{
    return frameTimeMs;
}
