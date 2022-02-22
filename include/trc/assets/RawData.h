#pragma once

#include <vector>
#include <unordered_map>

#include <glm/fwd.hpp>

#include "Types.h"
#include "Vertex.h"

namespace trc
{
    struct GeometryData
    {
        std::vector<Vertex> vertices;
        std::vector<VertexIndex> indices;
    };

    struct AnimationData
    {
        struct Keyframe
        {
            std::vector<mat4> boneMatrices;
        };

        std::string name;

        ui32 frameCount{ 0 };
        float durationMs{ 0.0f };
        float frameTimeMs{ 0.0f };
        std::vector<Keyframe> keyframes;
    };

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
        //
        // TODO: This information is redundant if I add a name to the Bone
        //       struct. Remove it.
        std::unordered_map<std::string, ui32> boneNamesToIndices;

        // A set of animations attached to the rig
        std::vector<AnimationData> animations;
    };



    struct TextureData
    {
        uvec2 size;
        std::vector<glm::u8vec4> pixels;
    };
} // namespace trc
