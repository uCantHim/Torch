#include "trc/assets/import/AssimpImporter.h"

#ifndef TRC_USE_ASSIMP

namespace trc::import
{

auto AssimpImporter::load(const fs::path& filePath) -> std::expected<ThirdPartyImport, ImportError>
{
    return std::unexpected(ImportError{
        filePath,
        ImportError::Code::eNotSupported,
        "Assimp import is not enabled, likely because assimp was not found during compilation."
    });
}

} // namespace trc::import

#else

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "trc/assets/import/GeometryTransformations.h"
#include "trc/base/Logging.h"



namespace trc::import
{

inline auto toVec4(aiColor4D c) -> basic_types::vec4
{
    return { c.r, c.g, c.b, c.a };
}

inline auto toVec3(aiVector3D v) -> basic_types::vec3
{
    return { v.x, v.y, v.z };
}

inline auto toVec2(aiVector2D v) -> basic_types::vec2
{
    return { v.x, v.y };
}

struct Loader
{
    auto loadAll(const fs::path& filePath) -> std::expected<ThirdPartyImport, ImportError>;

    auto loadMesh(const aiMesh* mesh) -> std::expected<GeometryImport, std::string>;
    auto loadMaterial(const aiMaterial* mat) -> std::expected<MaterialImport, std::string>;

    aiScene scene;
};

auto AssimpImporter::load(const fs::path& filePath)
    -> std::expected<ThirdPartyImport, ImportError>
{
    return Loader{}.loadAll(filePath);
}



auto Loader::loadAll(const fs::path& filePath) -> std::expected<ThirdPartyImport, ImportError>
{
    ThirdPartyImport result;
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
        log::error << "Unable to import assets from " << filePath << ": "
                   << importer.GetErrorString();
        return {};
    }

    for (ui32 i = 0; i < scene->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[i];
        auto geo = loadMesh(mesh);
        if (geo)
        {
            result.geometries.emplace_back(*geo);
            GeoID id{ result.geometries.size() - 1 };
            if (mesh->mMaterialIndex < scene->mNumMaterials) {
                result.refs.geoToMaterial.try_emplace(id, mesh->mMaterialIndex);
            }
        }
        else {
            log::error << "[AssimpImporter] Unable to import mesh \"" << mesh->mName.C_Str() << "\""
                       << ": " << geo.error() << ".";
        }
    }

    for (ui32 i = 0; i < scene->mNumMaterials; i++)
    {
        auto mat = loadMaterial(scene->mMaterials[i]);
        if (mat) {
            result.materials.emplace_back(*mat);
        }
        else {
            log::error << "[AssimpImporter] Unable to import material \""
                       << scene->mMaterials[i]->GetName().C_Str() << "\": " << mat.error() << ".";

            // Correct referenced material indices
            for (auto& [geo, mat] : result.refs.geoToMaterial)
            {
                if (mat > i) {
                    mat = MatID{ ui32{mat} - 1 };
                }
            }
        }
    }

    return result;
}

auto Loader::loadMesh(const aiMesh* mesh) -> std::expected<GeometryImport, std::string>
{
    if (!mesh->HasPositions() || !mesh->HasNormals()) {
        return std::unexpected("Mesh has no positions or no normals.");
    }

    GeometryImport newMesh;
    newMesh.name = mesh->mName.C_Str();
    auto& meshData = newMesh.data;

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
            log::warn << "[AssimpImporter] Unable to compute tangents for \"" << newMesh.name
                      << "\": Mesh has no texture coordinates.";
        }
    }

    // Load indices
    for (ui32 f = 0; f < mesh->mNumFaces; f++)
    {
        for (ui32 j = 0; j < mesh->mFaces[f].mNumIndices; j++) {
            meshData.indices.push_back(mesh->mFaces[f].mIndices[j]);
        }
    }

    return newMesh;
}

auto Loader::loadMaterial(const aiMaterial* mat) -> std::expected<MaterialImport, std::string>
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

    return MaterialImport{ .name=mat->GetName().C_Str(), .data=std::move(result) };
}

} // namespace trc::import

#endif
