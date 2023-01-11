#pragma once

#ifdef TRC_USE_FBX_SDK

#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <fbxsdk.h>

#include "trc/assets/import/AssetImportBase.h"

namespace trc
{
    namespace fs = std::filesystem;

    class FBXImporter
    {
    public:
        static auto load(const fs::path& path) -> ThirdPartyFileImportData;

    private:
        static void init();
        static inline FbxManager* fbx_memory_manager{ nullptr };
        static inline FbxIOSettings* fbx_io_settings{ nullptr };
        static inline bool initialized{ false };

        /** FBX data for one mesh */
        struct MeshImport
        {
            // Contructor needed by clang because that compiler is weird as fuck
            MeshImport(FbxMesh* fbxMesh, std::string name, mat4 transform)
                : fbxMesh(fbxMesh), name(std::move(name)), transform(transform)
            {}

            FbxMesh* fbxMesh;
            std::string name;
            mat4 transform;
        };

        /** Relevant FBX data of one scene */
        struct SceneImport
        {
            FbxScene* scene;
            std::vector<MeshImport> meshes;
            std::vector<FbxSkeleton*> skeletonRoots;
        };

        ///////////////////
        // FBX file loading
        static auto loadSceneFromFile(const std::string& path) -> std::optional<SceneImport>;

        // Ususal mesh data
        static auto loadMesh(FbxMesh* mesh) -> GeometryData;
        static void loadVertices(FbxMesh* mesh, GeometryData& result);
        static void loadUVs(FbxMesh* mesh, GeometryData& result);
        static void loadNormals(FbxMesh* mesh, GeometryData& result);
        static void loadTangents(FbxMesh* mesh, GeometryData& result);

        static auto loadMaterials(FbxMesh* mesh) -> std::vector<ThirdPartyMaterialImport>;

        // Animation data
        static constexpr ui32 MAX_WEIGHTS_PER_VERTEX = 4;

        /**
         * @brief Build a rig from a skeleton root node
         *
         * @return The created rig and an array of bone nodes. Entries in the bone node
         *         array correspond to the bone with the same index in the rig. This array
         *         is used internally to load animations.
         *         Also returns a map [bone name -> bone index].
         */
        static auto loadSkeleton(FbxSkeleton* skeleton)
            -> std::tuple<RigData, std::vector<FbxNode*>, std::unordered_map<std::string, ui32>>;

        /**
         * @brief Build a rig and loads that rig's bone indices and weights into a mesh
         *
         * @param GeometryData& result Weirdly, an out-parameter is sensible here
         */
        static auto loadRig(FbxMesh* mesh, GeometryData& result)
            -> std::pair<RigData, std::vector<FbxNode*>>;

        static auto loadAnimations(FbxScene* scene,
                                   const RigData& rig,
                                   const std::vector<FbxNode*>& boneNodes)
            -> std::vector<AnimationData>;

        static void correctBoneWeights(GeometryData& mesh);
    };
} // namespace trc

#endif // #ifdef TRC_USE_FBX_SDK
