#pragma once

#include <vector>
#include <optional>

#include "Types.h"
#include "Vertex.h"
#include "Assets.h"
#include "AssetReference.h"

namespace trc
{
    /**
     * @brief General data stored for every type of asset
     */
    struct AssetMetaData
    {
        std::string uniqueName;
    };

    struct GeometryData
    {
        std::vector<MeshVertex> vertices;
        std::vector<SkeletalVertex> skeletalVertices;
        std::vector<VertexIndex> indices;

        AssetReference<Rig> rig{};
    };

    struct TextureData
    {
        uvec2 size;
        std::vector<glm::u8vec4> pixels;
    };

    struct MaterialData
    {
        vec3 color{ 0.0f, 0.0f, 0.0f };

        vec4 ambientKoefficient{ 1.0f };
        vec4 diffuseKoefficient{ 1.0f };
        vec4 specularKoefficient{ 1.0f };

        float shininess{ 1.0f };
        float opacity{ 1.0f };
        float reflectivity{ 0.0f };

        bool doPerformLighting{ true };

        AssetReference<Texture> albedoTexture{};
        AssetReference<Texture> normalTexture{};
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
        std::vector<AssetReference<Animation>> animations;
    };
} // namespace trc
