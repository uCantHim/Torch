#pragma once

#include <mutex>

#include <vkb/VulkanBase.h>
#include <vkb/Buffer.h>

#include "Types.h"
#include "DescriptorProvider.h"

namespace trc
{
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

    class Animation
    {
    public:
        explicit Animation(const AnimationData& data);

        static auto getDescriptorProvider() noexcept -> DescriptorProviderInterface&;

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
        struct AnimationMeta
        {
            ui32 offset{ 0 };
            ui32 frameCount{ 0 };
            ui32 boneCount{ 0 };

            ui32 __padding{ 0 };
        };

        static constexpr size_t MAX_ANIMATIONS = 300;
        static constexpr size_t ANIMATION_BUFFER_SIZE = 2000000;

        static void vulkanStaticInit();
        static void vulkanStaticDestroy();
        static inline vkb::StaticInit _init{
            vulkanStaticInit, vulkanStaticDestroy
        };

        static inline std::mutex animationCreateLock;
        static inline ui32 numAnimations{ 0 };
        static inline ui32 animationBufferOffset{ 0 };
        static inline vkb::Buffer animationMetaDataBuffer;
        static inline vkb::Buffer animationBuffer;

        static void createDescriptors();
        static inline vk::DescriptorPool descPool;
        static inline vk::DescriptorSetLayout descLayout;
        static inline vk::DescriptorSet descSet;
        static inline DescriptorProvider descProvider{ {}, {} };

        ui32 animationIndex;
        ui32 frameCount;
        float durationMs;
        float frameTimeMs;
    };
} // namespace trc
