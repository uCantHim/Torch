#pragma once

#include <vector>

#include "Types.h"
#include "Vertex.h"

namespace trc
{
    struct GeometryData
    {
        std::string name;

        std::vector<MeshVertex> vertices;
        std::vector<SkeletalVertex> skeletalVertices;
        std::vector<VertexIndex> indices;
    };

    struct TextureData
    {
        std::string name;

        uvec2 size;
        std::vector<glm::u8vec4> pixels;
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
            std::string name;
            mat4 inverseBindPoseMat;
        };

        std::string name;

        // Indexed by per-vertex bone indices
        std::vector<Bone> bones;

        // A set of animations attached to the rig
        std::vector<AnimationData> animations;
    };
} // namespace trc
