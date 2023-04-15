#ifdef TRC_USE_ASSIMP

#include "trc/assets/import/AssimpImporter.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include "trc/assets/import/GeometryTransformations.h"
#include "trc/base/Logging.h"



inline auto toVec4(aiColor4D c) -> trc::basic_types::vec4
{
    return { c.r, c.g, c.b, c.a };
}

inline auto toVec3(aiVector3D v) -> trc::basic_types::vec3
{
    return { v.x, v.y, v.z };
}

inline auto toVec2(aiVector2D v) -> trc::basic_types::vec2
{
    return { v.x, v.y };
}

auto trc::AssetImporter::load(const fs::path& filePath) -> ThirdPartyFileImportData
{
    ThirdPartyFileImportData result;
    result.filePath = filePath;

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

auto trc::AssetImporter::loadMeshes(const aiScene* scene) -> std::vector<ThirdPartyMeshImport>
{
    std::vector<ThirdPartyMeshImport> result;

    for (ui32 i = 0; i < scene->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[i];
        if (mesh == nullptr) continue;
        if (!mesh->HasPositions() || !mesh->HasNormals())
        {
            log::error << "Unable to import mesh #" << i
                       << ": Mesh has no positions or no normals.\n";
            continue;
        }

        ThirdPartyMeshImport& newMesh = result.emplace_back();
        auto& meshData = newMesh.geometry;

        const bool hasUVs = mesh->HasTextureCoords(0);
        const bool hasTangents = mesh->HasTangentsAndBitangents();

        // Load vertices
        for (ui32 v = 0; v < mesh->mNumVertices; v++)
        {
            auto& vert = meshData.vertices.emplace_back(
                toVec3(mesh->mVertices[v]),   // position
                toVec3(mesh->mNormals[v]),    // normal
                vec2{},                       // uv
                vec3{}                        // tangent
            );
            if (hasUVs)      vert.uv = vec2(toVec3(mesh->mTextureCoords[0][v]));
            if (hasTangents) vert.tangent = toVec3(mesh->mTangents[v]);
        }

        // Compute tangents if not present in the imported data
        if (!hasTangents)
        {
            if (hasUVs) {
                computeTangents(meshData);
            }
            else {
                log::warn << "Unable to compute tangents: Mesh has no texture coordinates.";
            }
        }

        // Load indices
        for (ui32 f = 0; f < mesh->mNumFaces; f++)
        {
            for (ui32 j = 0; j < mesh->mFaces[f].mNumIndices; j++) {
                meshData.indices.push_back(mesh->mFaces[f].mIndices[j]);
            }
        }

        // Load material
        const aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
        newMesh.materials.push_back({ mat->GetName().C_Str(), loadMaterial(mat) });
    }

    return result;
}

auto trc::AssetImporter::loadMaterial(const aiMaterial* mat) -> SimpleMaterialData
{
    SimpleMaterialData result;

    aiColor4D color;

    mat->Get(AI_MATKEY_COLOR_DIFFUSE, color);
    result.color = vec3(toVec4(color));

    mat->Get(AI_MATKEY_SPECULAR_FACTOR, result.specularCoefficient);

    float opacity{ 1.0f };
    mat->Get(AI_MATKEY_OPACITY, opacity);
    result.opacity = opacity;

    // AI_MATKEY_ROUGHNESS_FACTOR is the exponent in the phong equation
    mat->Get(AI_MATKEY_ROUGHNESS_FACTOR, result.roughness);

    // AI_MATKEY_SHININESS_STRENGTH scales the specular color
    float shininessStrength{ 1.0f };
    mat->Get(AI_MATKEY_SHININESS_STRENGTH, shininessStrength);
    result.specularCoefficient *= shininessStrength;

    return result;
}

#endif
