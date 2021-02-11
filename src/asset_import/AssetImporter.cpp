#include "asset_import/AssetImporter.h"

#ifdef TRC_USE_ASSIMP

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>



inline auto toVec3(aiVector3D v) -> trc::basic_types::vec3
{
    return { v.x, v.y, v.z };
}

inline auto toVec2(aiVector2D v) -> trc::basic_types::vec2
{
    return { v.x, v.y };
}

auto trc::AssetImporter::load(const fs::path& filePath) -> FileImportData
{
    FileImportData result;

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(filePath.c_str(),
        aiProcess_Triangulate
        | aiProcess_JoinIdenticalVertices
        | aiProcess_GenSmoothNormals
        | aiProcess_CalcTangentSpace
        | aiProcess_GenUVCoords
    );

    if (!scene) {
        return {};
    }

    if (scene->HasMeshes()) {
        result.meshes = loadMeshes(scene);
    }

    return result;
}

auto trc::AssetImporter::loadMeshes(const aiScene* scene) -> std::vector<Mesh>
{
    std::vector<Mesh> result;

    for (ui32 i = 0; i < scene->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[i];
        Mesh& newMesh = result.emplace_back();
        auto& meshData = newMesh.mesh;

        // Load vertices
        for (ui32 v = 0; v < mesh->mNumVertices; v++)
        {
            meshData.vertices.emplace_back(
                toVec3(mesh->mVertices[v]),                // position
                toVec3(mesh->mNormals[v]),                 // normal
                vec2(toVec3(mesh->mTextureCoords[0][v])),  // uv
                toVec3(mesh->mTangents[v])                 // tangent
            );
        }

        // Load indices
        for (ui32 f = 0; f < mesh->mNumFaces; f++)
        {
            for (ui32 j = 0; j < mesh->mFaces[f].mNumIndices; j++) {
                meshData.indices.push_back(mesh->mFaces[f].mIndices[j]);
            }
        }
    }

    return result;
}

#endif
