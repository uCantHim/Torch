#include "assets/RigRegistry.h"



trc::RigRegistry::Handle::Handle(InternalStorage& storage)
    :
    storage(&storage)
{
}

auto trc::RigRegistry::Handle::getName() const noexcept -> const std::string&
{
    return storage->rigName;
}

auto trc::RigRegistry::Handle::getBoneByName(const std::string& name) const -> const RigData::Bone&
{
    return storage->bones.at(storage->boneNames.at(name));
}

auto trc::RigRegistry::Handle::getAnimationCount() const noexcept -> ui32
{
    return static_cast<ui32>(storage->animations.size());
}

auto trc::RigRegistry::Handle::getAnimation(ui32 index) const -> AnimationID
{
    return storage->animations.at(index);
}



trc::RigRegistry::InternalStorage::InternalStorage(const RigData& data)
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



trc::RigRegistry::RigRegistry(const AssetRegistryModuleCreateInfo&)
{
}

void trc::RigRegistry::update(vk::CommandBuffer)
{
}

auto trc::RigRegistry::getDescriptorLayoutBindings() -> std::vector<DescriptorLayoutBindingInfo>
{
    return {};
}

auto trc::RigRegistry::getDescriptorUpdates() -> std::vector<vk::WriteDescriptorSet>
{
    return {};
}

auto trc::RigRegistry::add(u_ptr<AssetSource<Rig>> source) -> LocalID
{
    const LocalID id{ rigIdPool.generate() };
    storage.emplace(id, std::make_unique<InternalStorage>(source->load()));

    return id;
}

void trc::RigRegistry::remove(LocalID id)
{
    storage.erase(id);
}

auto trc::RigRegistry::getHandle(LocalID id) -> Handle
{
    return Handle(*storage.get(id));
}
