#include "FrameClock.h"



vkb::FrameClock::FrameClock(uint32_t numFrames)
    :
    numFrames(numFrames)
{
}

auto vkb::FrameClock::getCurrentFrame() const -> uint32_t
{
    return currentFrame;
}

auto vkb::FrameClock::getFrameCount() const -> uint32_t
{
    return numFrames;
}

void vkb::FrameClock::resetCurrentFrame()
{
    currentFrame = 0;
}

void vkb::FrameClock::endFrame()
{
    currentFrame = (currentFrame + 1) % numFrames;
}
