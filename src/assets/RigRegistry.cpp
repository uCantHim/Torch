#include "assets/RigRegistry.h"

#include "assets/AssetManager.h"



namespace trc
{

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
    return storage->rigName;
}

auto AssetHandle<Rig>::getBoneByName(const std::string& name) const -> const RigData::Bone&
{
    return storage->bones.at(storage->boneNames.at(name));
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
    rigName(data.name),
    bones(data.bones)
{
    // Create mapping from bone name to bone index
    for (ui32 i = 0; const RigData::Bone& bone : data.bones)
    {
        boneNames[bone.name] = i++;
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
