#pragma once

#include <string>

#include "Rig.h"
#include "trc_util/data/ExternalStorage.h"

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

        AnimationEngine() = default;
        AnimationEngine(const Rig& rig);

        void update(float timeDeltaMs);

        void playAnimation(ui32 index);
        void playAnimation(const std::string& name);
        void playAnimation(const Animation& anim);

        auto getState() const -> ID;

    private:
        void resetAnimationTime();

        const Rig* rig{ nullptr };

        const Animation* currentAnimation{ nullptr };
        float currentDuration{ 0.0f };
        uvec2 currentFrames{ 0, 1 };

        data::ExternalStorage<AnimationDeviceData> animationState;
    };
} // namespace trc
