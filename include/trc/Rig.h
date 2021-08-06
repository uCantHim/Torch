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

        // A set of animations attached to the rig
        std::vector<AnimationData> animations;
    };

    class Rig
    {
    public:
        Rig(const RigData& data, AnimationDataStorage& animStorage);

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
         * @brief Get an index handle to an animation in the rig
         *
         * This is not the animation's index in the GPU buffer! This index
         * is used to query the animation from this rig.
         *
         * @return ui32
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

        auto addAnimation(const AnimationData& anim) -> ui32;

    private:
        AnimationDataStorage* animationStorage;

        std::string rigName;

        std::vector<RigData::Bone> bones;
        std::unordered_map<std::string, ui32> boneNames;

        std::vector<Animation> animations;
        std::unordered_map<std::string, ui32> animationNames;
    };
}
