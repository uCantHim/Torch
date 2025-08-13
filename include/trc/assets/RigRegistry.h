#pragma once

#include <iosfwd>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <componentlib/Table.h>
#include <trc_util/data/IdPool.h>

#include "trc/Types.h"
#include "trc/assets/AnimationRegistry.h"
#include "trc/assets/AssetManagerBase.h"  // For asset ID types
#include "trc/assets/AssetReference.h"
#include "trc/assets/AssetRegistryModule.h"
#include "trc/assets/AssetSource.h"

namespace trc
{
    class RigRegistry;

    struct Rig
    {
        using Registry = RigRegistry;

        static consteval auto name() -> std::string_view {
            return "torch_rig";
        }
    };

    template<>
    struct AssetData<Rig>
    {
        struct Bone
        {
            std::string name;
            mat4 inverseBindPoseMat;
        };

        std::string name;
        size_t numJoints;

        // Index of the root joint
        ui32 rootJoint;

        // For each joint its parent's index
        std::vector<std::optional<ui32>> parent;

        // For each joint a list of child joints
        std::vector<std::vector<ui32>> children;

        std::vector<mat4> inverseBindPoseMat;
        std::vector<mat4> localJointTransform;
        std::vector<mat4> jointTransform;  // Joint transform with respect to the skeleton root

        std::vector<std::string> jointName;

        // A set of animations attached to the rig
        std::vector<AssetReference<Animation>> animations;

        /**
         * Calculate the global transformation of a joint with respect to the
         * rig's root.
         *
         * Global joint transforms are usually precalculated during import - see
         * the `RidData::jointTransform` array. This method is useful if you
         * want to modify joints and recalculate their global transforms. It
         * does all of the tree walking for you.
         */
        auto calcJointTransform(ui32 jointIdx) const -> mat4
        {
            mat4 mat = localJointTransform[jointIdx];
            while (parent[jointIdx])
            {
                mat = localJointTransform[*parent[jointIdx]] * mat;
                jointIdx = *parent[jointIdx];
            }
            return mat;
        }

        void serialize(std::ostream& os) const;
        void deserialize(std::istream& is);

        void resolveReferences(AssetManager& man);
    };

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
        auto getBoneByName(const std::string& name) const -> AssetData<Rig>::Bone;

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
            InternalStorage(const AssetData<Rig>& data);

            AssetData<Rig> data;
            std::unordered_map<std::string, ui32> boneNames;

            std::vector<AnimationID> animations;
        };

        explicit AssetHandle(InternalStorage& storage);

        InternalStorage* storage;
    };

    using RigHandle = AssetHandle<Rig>;
    using RigData = AssetData<Rig>;
    using RigID = TypedAssetID<Rig>;

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

        data::IdPool<ui64> rigIdPool;
        componentlib::Table<u_ptr<InternalStorage>, LocalID> storage;
    };
} // namespace trc
