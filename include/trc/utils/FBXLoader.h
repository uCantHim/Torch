#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <optional>

#include <fbxsdk.h>

#include "Boilerplate.h"
#include "Geometry.h"

namespace trc
{
    struct RigData
    {
    };

    struct AnimationData
    {
    };

    struct Mesh
    {
        std::string name;
        mat4 globalTransform;

        MeshData mesh;
        RigData rig;
        std::vector<AnimationData> animations;
    };

    /**
     * @brief A holder for all data loaded from a file.
     */
    struct FileImportData
    {
        std::vector<Mesh> meshes;
    };

    class FBXLoader
    {
    public:
        FBXLoader();

        FBXLoader(const FBXLoader&) = default;
        FBXLoader(FBXLoader&&) = default;
        FBXLoader& operator=(const FBXLoader&) = default;
        FBXLoader& operator=(FBXLoader&&) noexcept = default;
        ~FBXLoader() = default;

        auto loadFBXFile(const std::string& path) -> FileImportData;

    private:
        static inline FbxManager* fbx_memory_manager{ nullptr };
        static inline FbxIOSettings* fbx_io_settings{ nullptr };
        static inline bool initialized{ false };

        /** FBX data for one mesh */
        struct MeshImport
        {
            FbxMesh* fbxMesh;
            std::string name;
            mat4 transform;
        };

        /** Relevant FBX data of one scene */
        struct SceneImport
        {
            std::vector<MeshImport> meshes;
            std::vector<FbxSkeleton*> skeletonRoots;
        };

        ///////////////////
        // FBX file loading
        auto loadSceneFromFile(const std::string& path) -> std::optional<SceneImport>;

        // Ususal mesh data
        static auto loadMesh(FbxMesh* mesh) -> MeshData;
        static void loadVertices(FbxMesh* mesh, MeshData& result);
        static void loadUVs(FbxMesh* mesh, MeshData& result);
        static void loadNormals(FbxMesh* mesh, MeshData& result);
        static void loadTangents(FbxMesh* mesh, MeshData& result);
        static void computeTangents(MeshData& result);

        // static void loadMaterials(FbxMesh* mesh, MeshConstructionParams* newMesh);

        // // Animation data
        // void loadSkeleton(FbxMesh* mesh, ImportResult* newMesh);
        // void createBonesFromSkeleton(FbxNode* currentBoneNode, AnimationBone* parent, AnimRigConstrParams* newRigParams);
        // void fillBoneData(FbxMesh* mesh, ImportResult* newMesh);

        std::map<std::string, int> nameToBoneIndex;

        FbxScene* scene{ nullptr };

        std::vector<FbxNode*> boneNodes;
    };
} // namespace trc
