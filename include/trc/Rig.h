#pragma once

#include "Animation.h"

namespace trc
{
    struct RigData
    {
        struct Bone
        {
            mat4 inverseBindPoseMat;
        };

        std::string name;

        // Indexed by per-vertex bone indices
        std::vector<Bone> bones;

        // Maps bone names to their indices in the bones array
        std::unordered_map<std::string, ui32> boneNamesToIndices;
    };

    class Rig
    {
    public:
        Rig(const RigData& data,
            AnimationDataStorage& animStorage,
            const std::vector<AnimationData>& animationData);

        auto getAnimationCount() const noexcept -> ui32;

        /**
         * @return Animation& The animation at the specified index
         * @throw std::out_of_range
         */
        auto getAnimation(ui32 index) -> Animation&;
        /**
         * @return const Animation& The animation at the specified index
         * @throw std::out_of_range
         */
        auto getAnimation(ui32 index) const -> const Animation&;

        /**
         * @return ui32 The index of the animation with the specified name
         * @throw std::out_of_range
         */
        auto getAnimationIndex(const std::string& name) const noexcept -> ui32;

        /**
         * @return Animation& The animation with the specified name
         * @throw std::out_of_range
         */
        auto getAnimationByName(const std::string& name) -> Animation&;
        /**
         * @return Animation& The animation with the specified name
         * @throw std::out_of_range
         */
        auto getAnimationByName(const std::string& name) const -> const Animation&;

    private:
        std::vector<RigData::Bone> bones;
        std::vector<Animation> animations;
        std::unordered_map<std::string, ui32> animationNames;
    };
}
