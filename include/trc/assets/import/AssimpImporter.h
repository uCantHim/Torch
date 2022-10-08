#pragma once

#include <filesystem>

#ifdef TRC_USE_ASSIMP
#include <assimp/scene.h>
#endif

#include "trc/assets/import/AssetImportBase.h"

#ifdef TRC_USE_ASSIMP

namespace trc
{
    namespace fs = std::filesystem;

    class AssetImporter
    {
    public:
        static auto load(const fs::path& filePath) -> ThirdPartyFileImportData;

    private:
        static auto loadMeshes(const aiScene* scene) -> std::vector<ThirdPartyMeshImport>;
        static auto loadMaterial(const aiMaterial* mat) -> MaterialData;
    };
} // namespace trc

#endif
