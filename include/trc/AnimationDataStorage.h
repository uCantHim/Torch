#pragma once

#include <mutex>

#include <vkb/Buffer.h>

#include "Types.h"
#include "DescriptorProvider.h"
#include "Animation.h"

namespace trc
{
    class Instance;

    /**
     * @brief GPU storage for animation data and descriptors
     *
     * Can create animation handles.
     */
    class AnimationDataStorage
    {
    public:
        explicit AnimationDataStorage(const Instance& instance);

        auto makeAnimation(const AnimationData& data) -> Animation;

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

        // Device memory management
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
