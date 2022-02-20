#pragma once

#include "AssetImportBase.h"

#ifdef TRC_USE_ASSIMP

#include <assimp/scene.h>

namespace trc
{
    class AssetImporter
    {
    public:
        static auto load(const fs::path& filePath) -> FileImportData;

    private:
        static auto loadMeshes(const aiScene* scene) -> std::vector<Mesh>;
        static auto loadMaterial(const aiMaterial* mat) -> Material;
    };
} // namespace trc

#endif
