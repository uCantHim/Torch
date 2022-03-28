#pragma once

#include <mutex>

#include <vkb/Buffer.h>
#include <componentlib/Table.h>
#include <trc_util/data/ObjectId.h>

#include "Types.h"
#include "AssetRegistryModule.h"
#include "Assets.h"
#include "AssetSource.h"
#include "assets/RawData.h"

namespace trc
{
    /**
     * @brief GPU storage for animation data and descriptors
     *
     * Can create animation handles.
     */
    class AnimationRegistry : public AssetRegistryModuleInterface
    {
    public:
        using LocalID = TypedAssetID<Animation>::LocalID;

        class Handle
        {
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
            friend class AnimationRegistry;
            Handle(const AnimationData& data, ui32 deviceIndex);

            /** Index in the AnimationDataStorage's large animation buffer */
            ui32 id;

            ui32 frameCount;
            float durationMs;
            float frameTimeMs;
        };

        explicit AnimationRegistry(const AssetRegistryModuleCreateInfo& info);

        void update(vk::CommandBuffer cmdBuf) final;

        auto getDescriptorLayoutBindings() -> std::vector<DescriptorLayoutBindingInfo> final;
        auto getDescriptorUpdates() -> std::vector<vk::WriteDescriptorSet> final;

        auto add(u_ptr<AssetSource<Animation>> source) -> LocalID;

        auto getHandle(LocalID id) -> Handle;

    private:
        struct AnimationMeta
        {
            ui32 offset{ 0 };
            ui32 frameCount{ 0 };
            ui32 boneCount{ 0 };
        };

        static constexpr size_t MAX_ANIMATIONS = 300;
        static constexpr size_t ANIMATION_BUFFER_SIZE = 2000000;

        const AssetRegistryModuleCreateInfo config;
        const vkb::Device& device;

        auto makeAnimation(const AnimationData& data) -> ui32;

        // Host resources
        componentlib::Table<Handle, LocalID> storage;
        data::IdPool animIdPool;

        // Device memory management
        std::mutex animationCreateLock;
        ui32 numAnimations{ 0 };
        ui32 animationBufferOffset{ 0 };

        vkb::Buffer animationMetaDataBuffer;
        vkb::Buffer animationBuffer;

        std::vector<vk::DescriptorBufferInfo> bufferInfos;
    };
} // namespace trc
