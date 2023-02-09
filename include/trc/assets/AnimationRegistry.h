#pragma once

#include <mutex>

#include "trc/base/Buffer.h"
#include <componentlib/Table.h>
#include <trc_util/data/IdPool.h>

#include "trc/Types.h"
#include "trc/assets/AssetRegistryModule.h"
#include "trc/assets/AssetSource.h"
#include "trc/assets/SharedDescriptorSet.h"

namespace trc
{
    class AnimationRegistry;

    struct Animation
    {
        using Registry = AnimationRegistry;

        static consteval auto name() -> std::string_view {
            return "torch_anim";
        }
    };

    template<>
    struct AssetData<Animation>
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

        void serialize(std::ostream& os) const;
        void deserialize(std::istream& is);
    };

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
        AssetHandle(const AssetData<Animation>& data, ui32 deviceIndex);

        /** Index in the AnimationDataStorage's large animation buffer */
        ui32 id;

        ui32 frameCount;
        float durationMs;
        float frameTimeMs;
    };

    using AnimationHandle = AssetHandle<Animation>;
    using AnimationData = AssetData<Animation>;
    using AnimationID = TypedAssetID<Animation>;

    struct AnimationRegistryCreateInfo
    {
        const Device& device;
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

        const Device& device;

        auto makeAnimation(const AnimationData& data) -> ui32;

        // Host resources
        componentlib::Table<AnimationHandle, LocalID> storage;
        data::IdPool<ui64> animIdPool;

        // Device memory management
        std::mutex animationCreateLock;
        ui32 numAnimations{ 0 };
        ui32 animationBufferOffset{ 0 };

        Buffer animationMetaDataBuffer;
        Buffer animationBuffer;

        SharedDescriptorSet::Binding metaBinding;
        SharedDescriptorSet::Binding dataBinding;
    };
} // namespace trc
