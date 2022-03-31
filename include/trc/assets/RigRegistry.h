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
    class RigRegistry : public AssetRegistryModuleInterface
    {
    private:
        struct InternalStorage;

    public:
        class Handle
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
            friend RigRegistry;
            explicit Handle(InternalStorage& storage);

            InternalStorage* storage;
        };

        using LocalID = TypedAssetID<Rig>::LocalID;

        explicit RigRegistry(const AssetRegistryModuleCreateInfo& info);

        void update(vk::CommandBuffer cmdBuf) final;

        auto add(u_ptr<AssetSource<Rig>> source) -> LocalID;
        void remove(LocalID id);

        auto getHandle(LocalID id) -> Handle;

    private:
        struct InternalStorage
        {
            InternalStorage(const RigData& data);

            std::string rigName;

            std::vector<RigData::Bone> bones;
            std::unordered_map<std::string, ui32> boneNames;

            std::vector<AnimationID> animations;
        };

        data::IdPool rigIdPool;
        componentlib::Table<u_ptr<InternalStorage>, LocalID> storage;
    };
} // namespace trc
