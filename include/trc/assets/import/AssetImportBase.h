#pragma once

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

#include <componentlib/Table.h>

#include "trc/Types.h"
#include "trc/assets/Animation.h"
#include "trc/assets/Geometry.h"
#include "trc/assets/Material.h"
#include "trc/assets/Rig.h"

namespace trc::import
{
    namespace fs = std::filesystem;

    struct MaterialImport
    {
        std::string name;
        SimpleMaterialData data;
    };

    struct TextureImport
    {
        std::string name;
        TextureData data;
    };

    struct GeometryImport
    {
        std::string name;
        mat4 globalTransform{ glm::identity<mat4>() };

        GeometryData data;
    };

    struct RigImport
    {
        std::string name;
        mat4 globalTransform{ glm::identity<mat4>() };

        RigData data;
    };

    struct AnimationImport
    {
        std::string name;
        AnimationData data;
    };

    using GeoID = data::TypesafeID<GeometryImport>;
    using RigID = data::TypesafeID<RigImport>;
    using AnimID = data::TypesafeID<AnimationImport>;
    using TexID = data::TypesafeID<TextureImport>;
    using MatID = data::TypesafeID<MaterialImport>;

    /**
     * @brief A holder for all data loaded from a file.
     */
    struct ThirdPartyImport
    {
        fs::path filePath;

        std::vector<GeometryImport> geometries;
        std::vector<RigImport> rigs;
        std::vector<AnimationImport> animations;
        std::vector<MaterialImport> materials;
        std::vector<TextureImport> textures;

        struct Associations
        {
            std::unordered_map<GeoID, RigID> geoToRig;
            std::unordered_map<RigID, std::vector<ui32>> rigToAnimations;
            std::unordered_map<GeoID, MatID> geoToMaterial;
            std::unordered_map<MatID, std::vector<TexID>> materialToTextures;
        };

        Associations refs;

        auto getRig(GeoID geo) -> RigImport*
        {
            auto it = refs.geoToRig.find(geo);
            if (it != refs.geoToRig.end()) {
                return &rigs[it->second];
            }
            return nullptr;
        }

        auto getAnimations(RigID rig) -> std::vector<AnimationImport*>
        {
            auto it = refs.rigToAnimations.find(rig);
            if (it != refs.rigToAnimations.end())
            {
                return it->second
                    | std::views::transform([this](auto id){ return &animations[id]; })
                    | std::ranges::to<std::vector>();
            }
            return {};
        }

        auto getMaterials(GeoID geo) -> std::vector<MaterialImport*>
        {
            auto it = refs.geoToMaterial.find(geo);
            if (it != refs.geoToMaterial.end()) {
                return { &materials[it->second] };
            }
            return {};
        }

        void bakeAssociations()
        {
            for (const auto& [rig, anims] : refs.rigToAnimations)
            {
                for (auto anim : anims) {
                    rigs[rig].data.animations.emplace_back(animations[anim].data);
                }
            }

            for (const auto& [geo, rig] : refs.geoToRig) {
                geometries[geo].data.rig = rigs[rig].data;
            }
        }

        /**
         * @return `nullptr` if no mesh with the specified name was found. A
         *         valid pointer otherwise.
         */
        auto findMesh(std::string_view name) -> GeometryImport*
        {
            auto it = std::ranges::find_if(geometries, [&](auto& mesh){ return mesh.name == name; });
            if (it != geometries.end()) {
                return &*it;
            }
            return nullptr;
        }
    };

    struct ImportError
    {
        enum class Code
        {
            eFilesystemError,
            eSyntaxError,
            eSemanticError,
            eNotSupported,
            eOther,
        };

        fs::path filePath;
        Code code;
        std::string msg;
    };
} // namespace trc::import
