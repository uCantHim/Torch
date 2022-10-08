#include "trc/base/FrameClock.h"



trc::FrameClock::FrameClock(uint32_t numFrames)
    :
    numFrames(numFrames)
{
}

auto trc::FrameClock::getCurrentFrame() const -> uint32_t
{
    return currentFrame;
}

auto trc::FrameClock::getFrameCount() const -> uint32_t
{
    return numFrames;
}

void trc::FrameClock::resetCurrentFrame()
{
    currentFrame = 0;
}

void trc::FrameClock::endFrame()
{
    currentFrame = (currentFrame + 1) % numFrames;
}
