#pragma once

#include <mutex>

#include <vkb/Buffer.h>
#include <componentlib/Table.h>
#include <trc_util/data/ObjectId.h>

#include "Types.h"
#include "AssetRegistryModule.h"
#include "AssetBaseTypes.h"
#include "AssetSource.h"
#include "SharedDescriptorSet.h"
#include "import/RawData.h"

namespace trc
{
    template<>
    class AssetHandle<Animation>
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
        AssetHandle(const AnimationData& data, ui32 deviceIndex);

        /** Index in the AnimationDataStorage's large animation buffer */
        ui32 id;

        ui32 frameCount;
        float durationMs;
        float frameTimeMs;
    };

    struct AnimationRegistryCreateInfo
    {
        const vkb::Device& device;
        SharedDescriptorSet::Builder& descriptorBuilder;
    };

    /**
     * @brief GPU storage for animation data and descriptors
     *
     * Can create animation handles.
     */
    class AnimationRegistry : public AssetRegistryModuleInterface<Animation>
    {
    public:
        using LocalID = TypedAssetID<Animation>::LocalID;

        explicit AnimationRegistry(const AnimationRegistryCreateInfo& info);

        void update(vk::CommandBuffer cmdBuf, FrameRenderState&) final;

        auto add(u_ptr<AssetSource<Animation>> source) -> LocalID override;
        void remove(LocalID) override {}

        auto getHandle(LocalID id) -> AnimationHandle override;

    private:
        struct AnimationMeta
        {
            ui32 offset{ 0 };
            ui32 frameCount{ 0 };
            ui32 boneCount{ 0 };
        };

        static constexpr size_t MAX_ANIMATIONS = 300;
        static constexpr size_t ANIMATION_BUFFER_SIZE = 2000000;

        const vkb::Device& device;

        auto makeAnimation(const AnimationData& data) -> ui32;

        // Host resources
        componentlib::Table<AnimationHandle, LocalID> storage;
        data::IdPool animIdPool;

        // Device memory management
        std::mutex animationCreateLock;
        ui32 numAnimations{ 0 };
        ui32 animationBufferOffset{ 0 };

        vkb::Buffer animationMetaDataBuffer;
        vkb::Buffer animationBuffer;

        SharedDescriptorSet::Binding metaBinding;
        SharedDescriptorSet::Binding dataBinding;
    };
} // namespace trc
