#include <cassert>

#include <optional>
#include <ranges>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <trc_util/TypeUtils.h>

#include "trc/Types.h"
#include "trc/assets/RigRegistry.h"
#include "trc/assets/import/AssetImportBase.h"
#include "trc/base/Logging.h"

namespace trc
{

using NodeID = data::TypesafeID<fastgltf::Node>;

using MeshID = import::GeoID;
using SkinID = import::RigID;
using AnimID = import::AnimID;



constexpr
auto toMat4(const fastgltf::math::fmat4x4& mat) -> glm::mat4
{
    mat4 res;
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j) {
            res[i][j] = mat[i][j];
        }
    }
    return res;
}



/**
 * A relic of the past, which I'm too lazy to remove.
 *
 * We use the names 'skin', 'skeleton', and 'rig' interchangeably. It ain't
 * pretty, but it works.
 */
using Skeleton = RigData;

auto loadSkeleton(const fastgltf::Asset& model, const fastgltf::Skin& skin) -> Skeleton
{
    const size_t numJoints = skin.joints.size();

    Skeleton skel;
    skel.name = skin.name;
    skel.numJoints = numJoints;
    skel.rootJoint = 0;
    skel.parent.resize(numJoints);
    skel.children.resize(numJoints);
    skel.inverseBindPoseMat.resize(numJoints);
    skel.localJointTransform.resize(numJoints);
    skel.jointTransform.resize(numJoints);
    skel.jointName.resize(numJoints);

    // Load node data
    std::unordered_map<ui32, ui32> nodeToJointIdx;
    fastgltf::iterateAccessorWithIndex<fastgltf::math::fmat4x4>(
        model, model.accessors.at(*skin.inverseBindMatrices),
        [&](fastgltf::math::fmat4x4 mat, ui32 jointIdx)
        {
            // Load the joint's matrices
            const auto& node = model.nodes.at(skin.joints.at(jointIdx));
            skel.inverseBindPoseMat[jointIdx] = toMat4(mat);
            skel.localJointTransform[jointIdx] = toMat4(fastgltf::getTransformMatrix(node));
            skel.jointName[jointIdx] = node.name;

            nodeToJointIdx[skin.joints[jointIdx]] = jointIdx;
        }
    );

    // Set child-parent relationships
    std::unordered_set<ui32> orphanJoints;
    std::unordered_set<ui32> jointsWithParent;
    for (ui32 jointIdx = 0; jointIdx < numJoints; ++jointIdx)
    {
        // Mark the joint as an orphan if it doesn't already have a parent
        if (!jointsWithParent.contains(jointIdx)) {
            orphanJoints.emplace(jointIdx);
        }

        const auto& node = model.nodes.at(skin.joints.at(jointIdx));
        for (const auto childNodeIdx : node.children)
        {
            // Skip if the child is not a joint
            if (!nodeToJointIdx.contains(childNodeIdx)) {
                continue;
            }

            const auto childIdx = nodeToJointIdx.at(childNodeIdx);
            orphanJoints.erase(childIdx);
            jointsWithParent.emplace(childIdx);

            skel.children[jointIdx].emplace_back(childIdx);
            skel.parent[childIdx] = jointIdx;
        }
    }

    // Set and validate root joint
    if (orphanJoints.size() > 1)
    {
        log::warn << "[GltfImporter] More than one possible candidate for root"
                     " joint of skeleton \"" << skin.name << "\"."
                     " Selecting \"" << model.nodes[*orphanJoints.begin()].name << "\".";
    }
    skel.rootJoint = *orphanJoints.begin();
    assert(!skel.parent[skel.rootJoint]);

    // Pre-calculate global joint transforms
    for (ui32 boneIdx = 0; boneIdx < skel.parent.size(); ++boneIdx) {
        skel.jointTransform[boneIdx] = skel.calcJointTransform(boneIdx);
    }

    return skel;
}

struct AnimationPath
{
    template<typename T>
    struct Keyframes
    {
        std::vector<float> times;
        std::vector<T> values;

        auto size() const -> size_t
        {
            assert(times.size() == values.size());
            return times.size();
        }

        void resize(size_t newSize)
        {
            times.resize(newSize, 0.0f);
            values.resize(newSize);
        }
    };

    NodeID targetNode;

    Keyframes<vec3> translations;
    Keyframes<quat> rotations;
    Keyframes<vec3> scales;

    /**
     * @return Number of keyframes in the animation path.
     */
    auto size() const -> size_t
    {
        assert(translations.size() == rotations.size() && "Has expandChannels been called on the path?");
        assert(translations.size() == scales.size() && "Has expandChannels been called on the path?");

        return translations.size();
    }

    /**
     * Get the transformation matrix at a specific keyframe.
     */
    auto getKeyframeMatrix(size_t keyframe) const -> mat4
    {
        assert(keyframe < translations.size());
        assert(keyframe < rotations.size());
        assert(keyframe < scales.size());

        constexpr auto rotate = [](const mat4& m, const quat& q) -> mat4 {
            return m * glm::toMat4(q);
        };

        auto t = translations.values[keyframe];
        auto r = rotations.values[keyframe];
        auto s = scales.values[keyframe];
        return glm::scale(rotate(glm::translate(glm::identity<mat4>(), t), r), s);
    }
};

struct AnimationInfo
{
    // Number of keyframes in the animation.
    ui32 numKeyframes;

    // Total duration in seconds.
    float duration;
};

auto collectsPaths(const fastgltf::Asset& model, const fastgltf::Animation& anim)
    -> std::unordered_map<NodeID, AnimationPath>
{
    std::unordered_map<NodeID, AnimationPath> nodeToPath;

    for (const auto& [idx, channel] : std::views::enumerate(anim.channels))
    {
        if (!channel.nodeIndex) {
            continue;
        }

        const auto& sampler = anim.samplers[channel.samplerIndex];
        const auto& inputAcc = model.accessors[sampler.inputAccessor];
        const auto& outputAcc = model.accessors[sampler.outputAccessor];
        assert(inputAcc.count == outputAcc.count);

        const NodeID nodeIdx{ *channel.nodeIndex };
        auto [it, _] = nodeToPath.try_emplace(nodeIdx);
        auto& path = it->second;

        assert(path.targetNode == NodeID::NONE || path.targetNode == *channel.nodeIndex);
        path.targetNode = nodeIdx;

        /**
         * Set a channel's timestamp values.
         *
         * We need to query these values for each channel individually (meaning
         * we can NOT just query timestamps from the first channel we encounter
         * and copy these to the other two channels) because different animation
         * channels can have a different number of keyframes, even when they
         * operate on the same node.
         */
        auto initTimestamps = [&](auto&& keyframes) {
            keyframes.resize(inputAcc.count);
            for (ui32 i = 0; const float time : fastgltf::iterateAccessor<float>(model, inputAcc))
            {
                keyframes.times[i] = time;
                ++i;
            }
        };

        using fquat = fastgltf::math::quat<float>;
        switch (channel.path)
        {
        case fastgltf::AnimationPath::Translation:
            initTimestamps(path.translations);
            for (ui32 i = 0;
                 const auto t : fastgltf::iterateAccessor<fastgltf::math::fvec3>(model, outputAcc))
            {
                path.translations.values[i] = { t.x(), t.y(), t.z() };
                ++i;
            }
            break;
        case fastgltf::AnimationPath::Rotation:
            initTimestamps(path.rotations);
            for (ui32 i = 0;
                 const fquat q : fastgltf::iterateAccessor<fquat>(model, outputAcc))
            {
                path.rotations.values[i] = glm::quat{ q.w(), q.x(), q.y(), q.z() };
                ++i;
            }
            break;
        case fastgltf::AnimationPath::Scale:
            initTimestamps(path.scales);
            for (ui32 i = 0;
                 const auto s : fastgltf::iterateAccessor<fastgltf::math::fvec3>(model, outputAcc))
            {
                path.scales.values[i] = { s.x(), s.y(), s.z() };
                ++i;
            }
            break;
        case fastgltf::AnimationPath::Weights:
            break;
        }
    }

    return nodeToPath;
}

/**
 * Expand channels in an animation path such that all channels have the same
 * number of keyframes.
 */
void expandChannels(AnimationPath& path, const std::vector<float>& targetTimes)
{
    assert(path.translations.times.front() == path.rotations.times.front());
    assert(path.translations.times.front() == path.scales.times.front());
    assert(path.translations.times.back() == path.rotations.times.back());
    assert(path.translations.times.back() == path.scales.times.back());

    auto expand = [&]<typename T>(AnimationPath::Keyframes<T>& path)
    {
        const size_t maxKeyframes = targetTimes.size();
        if (path.size() == maxKeyframes) {
            return;
        }

        AnimationPath::Keyframes<T> newFrames;
        size_t timeIdx{ 0 };
        for (const float targetTime : std::views::take(targetTimes, targetTimes.size() - 1))
        {
            while (targetTime >= path.times[timeIdx + 1]) ++timeIdx;

            const float curFrom = path.times[timeIdx];
            const float curTo = path.times[timeIdx + 1];
            assert(curTo > curFrom);

            const float posInCurInterval = (targetTime - curFrom) / (curTo - curFrom);
            assert(posInCurInterval >= 0.0f && posInCurInterval <= 1.0f);

            const T newVal = path.values[timeIdx] * posInCurInterval
                           + path.values[timeIdx + 1] * (1.0f - posInCurInterval);

            newFrames.times.emplace_back(targetTime);
            newFrames.values.emplace_back(newVal);
        }
        newFrames.times.emplace_back(targetTimes.back());
        newFrames.values.emplace_back(path.values.back());

        std::swap(path, newFrames);
    };

    expand(path.translations);
    expand(path.rotations);
    expand(path.scales);
    assert(path.translations.size() == path.rotations.size());
    assert(path.translations.size() == path.scales.size());
}

/**
 * Expand smaller paths to the largest in a set of paths, such that all paths
 * contain the same number of keyframes.
 *
 * @return Meta-info about the animation.
 */
auto expandChannels(std::unordered_map<NodeID, AnimationPath>& nodeToPath)
    -> AnimationInfo
{
    /**
     * Find the largest sequence of keyframe timepoints among all of the paths,
     * and use that as a reference timeline to which all other paths shall be
     * expanded.
     *
     * # Example
     *
     * Reference timeline:
     *   +----------------------------------+
     *   | 0    | 0.25 | 0.5  | 0.75 | 1    |
     *   +----------------------------------+
     *
     * Path:
     *   +-------------------------+
     *   | time  | 0   | 0.5 | 1   |
     *   +-------------------------+
     *   | value | 1   | 2   | 3   |
     *   +-------------------------+
     *
     *      v   expanded to   v
     *
     *   +------------------------------------------+
     *   | time  | 0    | 0.25 | 0.5  | 0.75 | 1    |
     *   +------------------------------------------+
     *   | value | 1    | 1.5  | 2    | 2.5  | 3    |
     *   +------------------------------------------+
     */
    const auto& referenceTimes = [&] -> auto& {
        size_t maxKeyframes{ 0 };
        NodeID reference{ 0 };
        for (auto& [idx, path] : nodeToPath)
        {
            const size_t numKeyframes = std::max({
                path.translations.times.size(),
                path.rotations.times.size(),
                path.scales.times.size(),
            });
            if (numKeyframes > maxKeyframes)
            {
                maxKeyframes = numKeyframes;
                reference = idx;
            }
        }

        const auto& path = nodeToPath.at(reference);
        if (path.translations.size() == maxKeyframes) {
            return path.translations.times;
        }
        if (path.rotations.size() == maxKeyframes) {
            return path.rotations.times;
        }
        if (path.scales.size() == maxKeyframes) {
            return path.scales.times;
        }
        std::unreachable();
    }();

    for (auto& [_, path] : nodeToPath) {
        expandChannels(path, referenceTimes);
    }

    return AnimationInfo{
        .numKeyframes = static_cast<ui32>(referenceTimes.size()),
        .duration = referenceTimes.back() - referenceTimes.front(),
    };
}

/**
 * Bake a skin's bind pose matrices into an animation's keyframes.
 */
void bakeSkinBindPose(const Skeleton& _skin,
                      AnimationData& anim,
                      const mat4 globalTransform = mat4{ 1.0f })
{
    // Create a copy of the skin. For each keyframe, we modify the local joint
    // transforms in the skin and use its `calcJointTransform` utility to walk
    // the skeleton and calculate each joint's global transform.
    Skeleton skin = _skin;

    for (auto& kf : anim.keyframes)
    {
        // Adjust local bone transforms according to the animation keyframe
        for (const auto& [jointIdx, animMat] : kf.boneMatrices | std::views::enumerate) {
            skin.localJointTransform[jointIdx] = animMat;
        }

        // Calculate global animated joint transforms
        for (auto [jointIdx, invBindMat] : std::views::enumerate(skin.inverseBindPoseMat))
        {
            kf.boneMatrices[jointIdx] = globalTransform
                                      * skin.calcJointTransform(jointIdx)
                                      * invBindMat;
        }
    }
}

} // namespace trc
