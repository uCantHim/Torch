#include "Rig.h"

#include "AnimationDataStorage.h"



trc::Rig::Rig(const RigData& data, AnimationDataStorage& animStorage)
    :
    animationStorage(&animStorage),
    rigName(data.name),
    bones(data.bones),
    boneNames(data.boneNamesToIndices)
{
    for (const auto& anim : data.animations) {
        addAnimation(anim);
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

auto trc::Rig::getAnimation(ui32 index) -> Animation&
{
    return animations.at(index);
}

auto trc::Rig::getAnimation(ui32 index) const -> const Animation&
{
    return animations.at(index);
}

auto trc::Rig::getAnimationIndex(const std::string& name) const noexcept -> ui32
{
    return animationsByName.at(name);
}

auto trc::Rig::getAnimationName(ui32 index) const -> const std::string&
{
    return animationNamesById.at(index);
}

auto trc::Rig::getAnimationByName(const std::string& name) -> Animation&
{
    return animations.at(animationsByName.at(name));
}

auto trc::Rig::getAnimationByName(const std::string& name) const -> const Animation&
{
    return animations.at(animationsByName.at(name));
}

auto trc::Rig::addAnimation(const AnimationData& animData) -> ui32
{
    // Restrictions to ensure that the animation is compatible with the rig
    assert(!animData.keyframes.empty());
    assert(animData.keyframes[0].boneMatrices.size() == bones.size());
    assert(!animationsByName.contains(animData.name));

    animations.emplace_back(animationStorage->makeAnimation(animData));
    const ui32 id = animations.size() - 1;
    animationsByName[animData.name] = id;
    animationNamesById[id] = animData.name;

    return id;
}
