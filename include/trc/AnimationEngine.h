#pragma once

#include <optional>

#include <trc_util/data/ExternalStorage.h>

#include "assets/Rig.h"
#include "assets/Animation.h"

namespace trc
{
    constexpr ui32 NO_ANIMATION = UINT32_MAX;

    struct AnimationDeviceData
    {
        ui32 currentAnimation { NO_ANIMATION };
        ui32 keyframes[2]     { 0, 0 };
        float keyframeWeight  { 0.0f };
    };

    class AnimationEngine
    {
    public:
        using ID = data::ExternalStorage<AnimationDeviceData>::ID;

        AnimationEngine(RigHandle rig);

        void update(float timeDeltaMs);

        void playAnimation(ui32 index);
        void playAnimation(AnimationHandle anim);

        auto getState() const -> ID;

    private:
        void resetAnimationTime();

        RigHandle rig;

        std::optional<AnimationHandle> currentAnimation{ std::nullopt };
        float currentDuration{ 0.0f };
        uvec2 currentFrames{ 0, 1 };

        data::ExternalStorage<AnimationDeviceData> animationState;
    };
} // namespace trc
