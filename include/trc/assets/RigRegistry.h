#pragma once

#include <vector>

#include <componentlib/Table.h>
#include <trc_util/data/ObjectId.h>

#include "Types.h"
#include "AssetBaseTypes.h"
#include "AssetRegistryModule.h"
#include "AssetSource.h"
#include "import/RawData.h"

namespace trc
{
    template<>
    class AssetHandle<Rig>
    {
    public:
        /**
         * @return const std::string& The rig's name
         */
        auto getName() const noexcept -> const std::string&;

        /**
         * @brief Query a bone from the rig
         *
         * @return const Bone&
         * @throw std::out_of_range
         */
        auto getBoneByName(const std::string& name) const -> const RigData::Bone&;

        /**
         * @return ui32 The number of animations attached to the rig
         */
        auto getAnimationCount() const noexcept -> ui32;

        /**
         * @return AnimationID The animation at the specified index
         * @throw std::out_of_range if index exceeds getAnimationCount()
         */
        auto getAnimation(ui32 index) const -> AnimationID;

    private:
        friend class RigRegistry;

        struct InternalStorage
        {
            InternalStorage(const RigData& data);

            std::string rigName;

            std::vector<RigData::Bone> bones;
            std::unordered_map<std::string, ui32> boneNames;

            std::vector<AnimationID> animations;
        };

        explicit AssetHandle(InternalStorage& storage);

        InternalStorage* storage;
    };

    class RigRegistry : public AssetRegistryModuleInterface<Rig>
    {
    public:
        using LocalID = TypedAssetID<Rig>::LocalID;

        RigRegistry() = default;

        void update(vk::CommandBuffer cmdBuf, FrameRenderState&) final;

        auto add(u_ptr<AssetSource<Rig>> source) -> LocalID override;
        void remove(LocalID id) override;

        auto getHandle(LocalID id) -> RigHandle override;

    private:
        using InternalStorage = AssetHandle<Rig>::InternalStorage;

        data::IdPool rigIdPool;
        componentlib::Table<u_ptr<InternalStorage>, LocalID> storage;
    };
} // namespace trc
