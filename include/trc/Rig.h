#pragma once

#include <vector>

#include "Types.h"
#include "Assets.h"
#include "assets/RawData.h"
#include "AnimationRegistry.h"

namespace trc
{
    class Rig
    {
    public:
        explicit Rig(const RigData& data);

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

        void addAnimation(AnimationID anim);

    private:
        std::string rigName;

        std::vector<RigData::Bone> bones;
        std::unordered_map<std::string, ui32> boneNames;

        std::vector<AnimationID> animations;
    };
}
