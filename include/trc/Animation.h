#pragma once

#include <mutex>

#include <vkb/StaticInit.h>
#include <vkb/Buffer.h>

#include "Types.h"
#include "DescriptorProvider.h"

namespace trc
{
    class Instance;
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

        ui32 animationIndex;
        ui32 frameCount;
        float durationMs;
        float frameTimeMs;
    };

    /**
     * @brief GPU storage for animation data and descriptors
     *
     * Can create animation handles.
     */
    class AnimationDataStorage
    {
    public:
        explicit AnimationDataStorage(const Instance& instance);

        auto addAnimation(const AnimationData& data) -> Animation;

        auto getProvider() const -> const DescriptorProviderInterface&;

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

        const Instance& instance;

        std::mutex animationCreateLock;
        ui32 numAnimations{ 0 };
        ui32 animationBufferOffset{ 0 };

        vkb::Buffer animationMetaDataBuffer;
        vkb::Buffer animationBuffer;

        // Descriptor
        void createDescriptor(const Instance& instance);
        void writeDescriptor(const Instance& instance);

        vk::UniqueDescriptorPool descPool;
        vk::UniqueDescriptorSetLayout descLayout;
        vk::UniqueDescriptorSet descSet;
        DescriptorProvider descProvider{ {}, {} };
    };
} // namespace trc
