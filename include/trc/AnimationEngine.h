#pragma once

#include <vkb/util/Timer.h>

#include "Rig.h"

namespace trc
{
    constexpr ui32 NO_ANIMATION = UINT32_MAX;

    class AnimationEngine
    {
    public:
        AnimationEngine() = default;
        AnimationEngine(const Rig& rig);

        void playAnimation(ui32 index);
        void playAnimation(const std::string& name);
        void playAnimation(const Animation& anim);

        auto getCurrentAnimationIndex() const noexcept -> ui32;
        auto getCurrentFrameWeight() noexcept -> float;
        auto getCurrentFrames() noexcept -> uvec2;

        void pushConstants(ui32 offset, vk::PipelineLayout layout, vk::CommandBuffer cmdBuf);

    private:
        const Rig* rig{ nullptr };
        vkb::Timer<std::chrono::milliseconds> keyframeTimer;

        ui32 currentAnimationIndex{ NO_ANIMATION };
        const Animation* currentAnimation{ nullptr };
        uvec2 currentFrames{ 0, 1 };
    };
} // namespace trc
