#pragma once

#include "Types.h"

namespace trc
{
    class AnimationDataStorage;
    struct AnimationData;

    /**
     * @brief A handle to an animation
     */
    class Animation
    {
    public:
        Animation(const Animation&) = default;
        Animation(Animation&&) noexcept = default;
        auto operator=(const Animation&) -> Animation& = default;
        auto operator=(Animation&&) noexcept -> Animation& = default;

        Animation(AnimationDataStorage& storage, const AnimationData& data);

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
