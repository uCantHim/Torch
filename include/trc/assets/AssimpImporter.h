#pragma once

#include <filesystem>

#include "AssetImportBase.h"

#ifdef TRC_USE_ASSIMP

#include <assimp/scene.h>

namespace trc
{
    namespace fs = std::filesystem;

    class AssetImporter
    {
    public:
        static auto load(const fs::path& filePath) -> ThirdPartyFileImportData;

    private:
        static auto loadMeshes(const aiScene* scene) -> std::vector<ThirdPartyMeshImport>;
        static auto loadMaterial(const aiMaterial* mat) -> Material;
    };
} // namespace trc

#endif
