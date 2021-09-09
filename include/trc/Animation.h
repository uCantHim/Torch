#pragma once

#include <mutex>

#include <vkb/Buffer.h>

#include "Types.h"
#include "DescriptorProvider.h"

namespace trc
{
    class AnimationDataStorage;

    struct AnimationData
    {
        struct Keyframe
        {
            std::vector<mat4> boneMatrices;
        };

        std::string name;

        ui32 frameCount{ 0 };
        float durationMs{ 0.0f };
        float frameTimeMs{ 0.0f };
        std::vector<Keyframe> keyframes;
    };

    /**
     * @brief A handle to an animation
     */
    class Animation
    {
    private:
        friend class AnimationDataStorage;

        Animation(ui32 animationIndex, const AnimationData& data);

    public:
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
