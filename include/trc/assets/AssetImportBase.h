#pragma once

#include <string>
#include <vector>
#include <optional>
#include <filesystem>

#include "Types.h"
#include "assets/RawData.h"

namespace trc
{
    namespace fs = std::filesystem;

    struct ThirdPartyMeshImport
    {
        std::string name;
        mat4 globalTransform;

        GeometryData geometry;
        std::vector<MaterialData> materials;
        std::optional<RigData> rig;
    };

    /**
     * @brief A holder for all data loaded from a file.
     */
    struct ThirdPartyFileImportData
    {
        fs::path filePath;

        std::vector<ThirdPartyMeshImport> meshes;
    };
} // namespace trc
