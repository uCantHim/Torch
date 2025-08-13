#include "trc/assets/RigRegistry.h"

#include "trc/assets/AssetManager.h"
#include "trc/assets/import/InternalFormat.h"



namespace trc
{

void AssetData<Rig>::serialize(std::ostream& os) const
{
    serial::Rig rig = internal::serializeAssetData(*this);
    rig.SerializeToOstream(&os);
}

void AssetData<Rig>::deserialize(std::istream& is)
{
    serial::Rig rig;
    rig.ParseFromIstream(&is);
    *this = internal::deserializeAssetData(rig);
}

void AssetData<Rig>::resolveReferences(AssetManager& man)
{
    for (auto& ref : animations)
    {
        if (!ref.empty()) {
            ref.resolve(man);
        }
    }
}



AssetHandle<Rig>::AssetHandle(InternalStorage& storage)
    :
    storage(&storage)
{
}

auto AssetHandle<Rig>::getName() const noexcept -> const std::string&
{
    return storage->data.name;
}

auto AssetHandle<Rig>::getBoneByName(const std::string& name) const -> RigData::Bone
{
    ui32 boneIdx = storage->boneNames.at(name);
    return {
        .name = storage->data.jointName[boneIdx],
        .inverseBindPoseMat = storage->data.inverseBindPoseMat[boneIdx],
    };
}

auto AssetHandle<Rig>::getAnimationCount() const noexcept -> ui32
{
    return static_cast<ui32>(storage->animations.size());
}

auto AssetHandle<Rig>::getAnimation(ui32 index) const -> AnimationID
{
    return storage->animations.at(index);
}



AssetHandle<Rig>::InternalStorage::InternalStorage(const RigData& data)
    :
    data(data)
{
    // Create mapping from bone name to bone index
    for (auto [i, name] : std::views::enumerate(data.jointName)) {
        boneNames[name] = i;
    }

    for (const auto& anim : data.animations) {
        animations.emplace_back(anim.getID());
    }
}



void RigRegistry::update(vk::CommandBuffer, FrameRenderState&)
{
}

auto RigRegistry::add(u_ptr<AssetSource<Rig>> source) -> LocalID
{
    const LocalID id{ rigIdPool.generate() };
    storage.emplace(id, std::make_unique<InternalStorage>(source->load()));

    return id;
}

void RigRegistry::remove(LocalID id)
{
    storage.erase(id);
}

auto RigRegistry::getHandle(LocalID id) -> Handle
{
    return Handle(*storage.get(id));
}

} // namespace trc
