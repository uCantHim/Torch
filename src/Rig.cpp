#include "Rig.h"

#include "AssetManager.h"
#include "AnimationRegistry.h"



trc::Rig::Rig(const RigData& data)
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
        addAnimation(anim.getID());
    }
}

auto trc::Rig::getName() const noexcept -> const std::string&
{
    return rigName;
}

auto trc::Rig::getBoneByName(const std::string& name) const -> const RigData::Bone&
{
    return bones.at(boneNames.at(name));
}

auto trc::Rig::getAnimationCount() const noexcept -> ui32
{
    return static_cast<ui32>(animations.size());
}

auto trc::Rig::getAnimation(ui32 index) const -> AnimationID
{
    return animations.at(index);
}

void trc::Rig::addAnimation(AnimationID anim)
{
    animations.emplace_back(anim);
}
