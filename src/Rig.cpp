#include "Rig.h"



trc::Rig::Rig(const RigData& data, const std::vector<AnimationData>& animationData)
    :
    bones(data.bones)
{
    for (ui32 i = 0; const auto& anim : animationData)
    {
        animations.emplace_back(anim);
        animationNames[anim.name] = i++;
    }
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
    return animationNames.at(name);
}

auto trc::Rig::getAnimationByName(const std::string& name) -> Animation&
{
    return animations.at(animationNames.at(name));
}

auto trc::Rig::getAnimationByName(const std::string& name) const -> const Animation&
{
    return animations.at(animationNames.at(name));
}
