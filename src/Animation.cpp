#include "Animation.h"

#include "assets/RawData.h"



trc::AnimationDeviceHandle::AnimationDeviceHandle(const AnimationData& data, ui32 deviceIndex)
    :
    id(deviceIndex),
    frameCount(data.frameCount),
    durationMs(data.durationMs),
    frameTimeMs(data.frameTimeMs)
{
}

auto trc::AnimationDeviceHandle::getBufferIndex() const noexcept -> ui32
{
    return id;
}

auto trc::AnimationDeviceHandle::getFrameCount() const noexcept -> ui32
{
    return frameCount;
}

auto trc::AnimationDeviceHandle::getDuration() const noexcept -> float
{
    return durationMs;
}

auto trc::AnimationDeviceHandle::getFrameTime() const noexcept -> float
{
    return frameTimeMs;
}
