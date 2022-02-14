#include "Animation.h"

#include "AnimationDataStorage.h"



trc::Animation::Animation(AnimationDataStorage& storage, const AnimationData& data)
    :
    id(storage.makeAnimation(data)),
    frameCount(data.frameCount),
    durationMs(data.durationMs),
    frameTimeMs(data.frameTimeMs)
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
