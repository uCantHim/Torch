#pragma once

#include "Types.h"

namespace trc
{
    struct AnimationData;

    class AnimationDeviceHandle
    {
    private:
        friend class AnimationRegistry;
        AnimationDeviceHandle(const AnimationData& data, ui32 deviceIndex);

    public:
        auto getBufferIndex() const noexcept -> ui32;

        /**
         * @return uint32_t The number of frames in the animation
         */
        auto getFrameCount() const noexcept -> ui32;

        /**
         * @return float The total duration of the animation in milliseconds
         */
        auto getDuration() const noexcept -> float;

        /**
         * @return float The duration of a single frame in the animation in
         *               milliseconds. All frames in an animation have the same
         *               duration.
         */
        auto getFrameTime() const noexcept -> float;

    private:
        /** Index in the AnimationDataStorage's large animation buffer */
        ui32 id;

        ui32 frameCount;
        float durationMs;
        float frameTimeMs;
    };
} // namespace trc
