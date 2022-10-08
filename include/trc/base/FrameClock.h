#pragma once

#include <cstdint>

namespace trc
{
    class FrameClock
    {
    public:
        explicit FrameClock(uint32_t numFrames);

        auto getCurrentFrame() const -> uint32_t;
        auto getFrameCount() const -> uint32_t;

        /**
         * @brief Set the current frame to the first frame
         */
        void resetCurrentFrame();
        void endFrame();

    private:
        uint32_t numFrames;
        uint32_t currentFrame{ 0 };
    };
} // namespace trc
